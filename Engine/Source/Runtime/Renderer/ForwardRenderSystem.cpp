#include "Renderer/ForwardRenderSystem.h"
#include "Renderer/Material.h"
#include "Renderer/MaterialSchema.h"
#include "Renderer/BindlessTextures.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"
#include "Core/Application.h"
#include "Core/Input.h"
#include "Events/KeyCodes.h"
#include "Scene/World.h"
#include "Math/Vector.h"

namespace Yogi
{

ForwardRenderSystem::ForwardRenderSystem()
{
    m_vertexStorageBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_VERTICES_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshletBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshletDataBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_DATA_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshDrawBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESH_DRAW_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectBuffer = ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{
        MAX_INDIRECT_DRAW_COMMAND_SIZE, BufferUsage::Storage | BufferUsage::Indirect, BufferAccess::Dynamic });
    m_visibleDrawIndexBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_VISIBLE_DRAW_INDEX_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    m_meshTaskIndirectCountBuffer = ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{
        MAX_INDIRECT_DRAW_COUNT_SIZE, BufferUsage::Storage | BufferUsage::Indirect, BufferAccess::Dynamic });
    m_objectVisBuffer             = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_OBJECT_VIS_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    // Per-instance per-meshlet visibility -- one uint per (drawIndex, miLocal) slot.
    // Persistent across frames; LATE meshlet task is the sole writer.
    m_meshletVisBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MESHLET_VIS_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    // Bindless material data, uploaded fresh each frame from the collect loop.
    m_materialBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ MAX_MATERIAL_SIZE, BufferUsage::Storage, BufferAccess::Dynamic });
    // Per-frame static data (SceneFrame) accessed via BDA — sub-allocated each
    // frame from a single arena buffer so frame N's host writes don't race
    // frame N-1's GPU reads.
    {
        auto               swapChain    = Application::GetInstance().GetSwapChain();
        const uint32_t     imageCount   = swapChain->GetImageCount();
        constexpr uint64_t kSegmentSize = 4 * 1024;
        m_frameArena.Init(kSegmentSize, imageCount);
    }
    // Zero-init: first frame's EARLY skips everything (USE_PREV_VIS gate fails for all),
    // LATE then sees an empty pyramid (all depth = 1.0) and emits every visible draw.
    {
        std::vector<uint32_t> zeros(MAX_MESH_DRAWS, 0u);
        m_objectVisBuffer->UpdateData(zeros.data(), static_cast<uint32_t>(MAX_OBJECT_VIS_SIZE), 0);
    }
    // Same zero-init semantics for per-meshlet visibility: first frame, EARLY task
    // sees prev_meshlet_vis==0 for everything -> emits nothing, LATE task does the
    // full cone+frustum+Hi-Z and writes the canonical state. From frame 2 onward
    // EARLY emits whatever was visible last frame, LATE only re-tests the rest.
    //
    // 32M uints = 128 MB upload via UpdateData: this only runs at boot. UpdateData
    // is a staging-buffer copy; it does NOT block the render loop.
    {
        std::vector<uint32_t> zeros(MAX_MESHLET_VIS_COUNT, 0u);
        m_meshletVisBuffer->UpdateData(zeros.data(), static_cast<uint32_t>(MAX_MESHLET_VIS_SIZE), 0);
    }

    // Mesh/task/frag and cull pipelines share the engine-level BindlessTextures
    // SRB. Material textures + the pyramid sampled view (used by LATE Hi-Z)
    // both live in that pool; per-pipeline state flows through push constants
    // (slot indices, DrawBase, ...). The DepthReduce pipeline below is the
    // exception -- it owns private storage write targets so it brings its own
    // SRB chain instead of going through this pool.
    if (!BindlessTextures::IsInitialized())
        BindlessTextures::Initialize();
    IShaderResourceBinding* bindlessSRB = BindlessTextures::Get().GetSRB();

    WRef<ShaderDesc> cullShaderEarly = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/ObjectCull.comp");
    WRef<ShaderDesc> cullShaderLate =
        AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/ObjectCull.comp::LATE=1");

    PipelineDesc cullPipelineDesc{};
    cullPipelineDesc.Type               = PipelineType::Compute;
    cullPipelineDesc.ResourceBinding    = bindlessSRB;
    cullPipelineDesc.PushConstantRanges = { PushConstantRange{
        ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(CullPush)) } };

    cullPipelineDesc.Shaders = { cullShaderEarly.Get() };
    m_cullPipelineEarly      = ResourceManager::AcquireSharedResource<IPipeline>(cullPipelineDesc);

    cullPipelineDesc.Shaders = { cullShaderLate.Get() };
    m_cullPipelineLate       = ResourceManager::AcquireSharedResource<IPipeline>(cullPipelineDesc);

    // Meshlet (task + mesh + frag) graphics pipelines.
    {
        auto             swapChain       = Application::GetInstance().GetSwapChain();
        WRef<ShaderDesc> taskShaderEarly = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Test.task");
        WRef<ShaderDesc> taskShaderLate =
            AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Test.task::LATE=1");
        WRef<ShaderDesc> meshShader     = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Test.mesh");
        WRef<ShaderDesc> fragmentShader = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Test.frag");

        PipelineDesc meshletPipelineDesc{};
        meshletPipelineDesc.ResourceBinding    = bindlessSRB;
        meshletPipelineDesc.PushConstantRanges = { PushConstantRange{ ShaderStage::Task | ShaderStage::Mesh |
                                                                          ShaderStage::Fragment,
                                                                      0,
                                                                      static_cast<uint32_t>(sizeof(ScenePush)) } };
        meshletPipelineDesc.ColorFormats       = { swapChain->GetColorFormat() };
        meshletPipelineDesc.DepthFormat        = ITexture::Format::D32_FLOAT;
        meshletPipelineDesc.Samples            = SampleCountFlagBits::Count1;
        meshletPipelineDesc.Topology           = PrimitiveTopology::TriangleList;

        meshletPipelineDesc.Shaders = { taskShaderEarly.Get(), meshShader.Get(), fragmentShader.Get() };
        m_meshletEarlyPipeline      = ResourceManager::AcquireSharedResource<IPipeline>(meshletPipelineDesc);

        meshletPipelineDesc.Shaders = { taskShaderLate.Get(), meshShader.Get(), fragmentShader.Get() };
        m_meshletLatePipeline       = ResourceManager::AcquireSharedResource<IPipeline>(meshletPipelineDesc);
    }

    // Depth Pyramid (Hi-Z) compute pipeline. NOT bindless: the reduce pass
    // owns its storage-image write targets and their immediate sampled
    // sources, so it carries its own SRB layout (one descriptor set instance
    // per mip is allocated by DepthPyramid). Push-constant ranges only carry
    // the destination mip extent.
    WRef<IShaderResourceBinding> depthReduceBindingTemplate =
        ResourceManager::AddResource(DepthPyramid::CreateReduceBindingLayout());
    WRef<ShaderDesc> depthReduceShader =
        AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/DepthReduce.comp");
    PipelineDesc depthReducePipelineDesc{};
    depthReducePipelineDesc.Type               = PipelineType::Compute;
    depthReducePipelineDesc.Shaders            = { depthReduceShader.Get() };
    depthReducePipelineDesc.ResourceBinding    = depthReduceBindingTemplate.Get();
    depthReducePipelineDesc.PushConstantRanges = DepthPyramid::GetReducePushConstantRanges();
    m_depthReducePipeline = ResourceManager::AcquireSharedResource<IPipeline>(depthReducePipelineDesc);

    // Register the engine-wide MaterialSchema. Must mirror struct MaterialData
    // in ShaderData.h field-by-field. Adding a new PBR field is a 2-step:
    //   1. add it to MaterialData (ShaderData.h)
    //   2. add one AddField call below with its default
    // Materials reference fields by name and inherit the default if unset.
    {
        auto& schema = MaterialSchema::Default();
        schema.AddField("BaseColor",
                        offsetof(MaterialData, BaseColor),
                        MaterialSchema::FieldType::Vec4,
                        Vector4{ 1.0f, 1.0f, 1.0f, 1.0f });
        // Bindless texture index. Default 0 = the engine's 1x1 white view in
        // slot 0 of u_textures[]; any Material setting Params["AlbedoTexture"]
        // = WRef<ITexture>{...} gets the texture registered through
        // RegisterTexture and packed as the resolved uint slot.
        schema.AddField("AlbedoTexture", offsetof(MaterialData, AlbedoTexture), MaterialSchema::FieldType::Texture, 0u);
        schema.Build(static_cast<uint32_t>(sizeof(MaterialData)));
    }
}

ForwardRenderSystem::~ForwardRenderSystem()
{
    ResetMeshUploadCache();

    m_vertexStorageBuffer         = nullptr;
    m_meshletBuffer               = nullptr;
    m_meshletDataBuffer           = nullptr;
    m_meshDrawBuffer              = nullptr;
    m_meshTaskIndirectBuffer      = nullptr;
    m_visibleDrawIndexBuffer      = nullptr;
    m_meshTaskIndirectCountBuffer = nullptr;
    m_objectVisBuffer             = nullptr;
    m_meshletVisBuffer            = nullptr;
    m_materialBuffer              = nullptr;
    m_frameArena.Shutdown();
    m_cullPipelineEarly    = nullptr;
    m_cullPipelineLate     = nullptr;
    m_meshletEarlyPipeline = nullptr;
    m_meshletLatePipeline  = nullptr;
    m_depthReducePipeline  = nullptr;
    m_depthPyramid.Reset();
    m_depthView    = nullptr;
    m_depthTexture = nullptr;

    // Drop cached views. Unregister each from the bindless pool first so the
    // freed slots return to the LIFO free-list.
    if (BindlessTextures::IsInitialized())
    {
        BindlessTextures& bt = BindlessTextures::Get();
        for (auto& [tex, view] : m_materialViews)
        {
            const uint32_t slot = bt.Find(view.Get());
            if (slot != BindlessTextures::INVALID_SLOT)
                bt.Unregister(slot);
        }
    }
    m_materialViews.clear();
}

void ForwardRenderSystem::OnUpdate(Timestep ts, World& world)
{
    world.ViewComponents<TransformComponent, CameraComponent>(
        [&](Entity entity, TransformComponent& transform, CameraComponent& camera) {
            RenderCamera(camera, transform, world);
        });
}

void ForwardRenderSystem::OnEvent(Event& e, World& world)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>(YG_BIND_FN(ForwardRenderSystem::OnWindowResize, std::placeholders::_2),
                                           world);
}

bool ForwardRenderSystem::OnWindowResize(WindowResizeEvent& e, World& world)
{
    auto swapChain = Application::GetInstance().GetSwapChain();
    swapChain->AcquireCurrentCommandBuffer()->Wait();
    m_depthView    = nullptr;
    m_depthTexture = nullptr;
    // Depth Pyramid is tied to the (old) depth resolution, so force a rebuild on next frame.
    m_depthPyramid.Reset();
    return false;
}

void ForwardRenderSystem::ResetMeshUploadCache()
{
    m_meshUploadCache.Clear();
    m_cachedVertexCount     = 0;
    m_cachedMeshletCount    = 0;
    m_cachedMeshletDataSize = 0;
    // Do NOT clear m_cachedMeshMeshletCounts: it's indexed by meshIndex
    // (m_cachedMeshCount), which keeps growing across mid-frame flushes -- old
    // entries are stale but not in the way; FlushBatch only reads positions
    // [meshIndex] for live MeshDraws, all of which have valid entries appended
    // when the upload cache misses re-allocate them.
}

void ForwardRenderSystem::RenderCamera(const CameraComponent& camera, const TransformComponent& transform, World& world)
{
    YG_PROFILE_FUNCTION();

    auto            swapChain     = Application::GetInstance().GetSwapChain();
    ICommandBuffer* commandBuffer = swapChain->AcquireCurrentCommandBuffer().Get();
    ITextureView*   currentTarget = swapChain->AcquireCurrentTarget().Get();
    if (camera.Target)
    {
        currentTarget = camera.Target.Get();
    }
    const ITexture* targetTex = currentTarget->GetTexture();
    EnsureDepthTexture(targetTex->GetWidth(), targetTex->GetHeight());

    // When rendering directly to a swapchain image (no camera RT target), this
    // FlushBatch is the last cmd recording on that image -- it must drop the
    // image into PRESENT_SRC before Submit() so vkQueuePresentKHR is happy.
    // Otherwise (Editor with camera.Target set) the downstream layer (ImGui)
    // is responsible for issuing its own PRESENT barrier.
    const bool transitionToPresent = !camera.Target;

    // update scene data
    Matrix4 viewMatrix = MathUtils::Inverse(transform.Transform);
    Matrix4 projectionMatrix;
    float   aspectRatio = (float)targetTex->GetWidth() / (float)targetTex->GetHeight();
    if (camera.IsOrtho)
    {
        projectionMatrix = MathUtils::Orthographic(-aspectRatio * camera.ZoomLevel,
                                                   aspectRatio * camera.ZoomLevel,
                                                   -camera.ZoomLevel,
                                                   camera.ZoomLevel,
                                                   -1.0f,
                                                   1.0f);
    }
    else
    {
        projectionMatrix = MathUtils::Perspective(camera.Fov, aspectRatio, k_zNear, k_zFar);
    }
    SceneFrame sceneFrame{};
    sceneFrame.ProjectionViewMatrix = projectionMatrix * viewMatrix;
    sceneFrame.ViewMatrix           = viewMatrix;
    sceneFrame.ScreenSize = { static_cast<float>(targetTex->GetWidth()), static_cast<float>(targetTex->GetHeight()) };

    // niagara-style symmetric frustum: derive (Lx, Lz, Ty, Tz) from the projection
    // matrix's left and top planes (rows 3+0 and 3+1 of the transpose). Right/Bottom
    // are mirrored via abs() in the cull/task shaders.
    auto    PT         = projectionMatrix.Transpose();
    Vector4 leftRaw    = PT[3] + PT[0];
    Vector4 topRaw     = PT[3] + PT[1];
    Vector3 leftN      = Vector3(leftRaw.x, leftRaw.y, leftRaw.z);
    Vector3 topN       = Vector3(topRaw.x, topRaw.y, topRaw.z);
    float   leftLen    = leftN.Length();
    float   topLen     = topN.Length();
    Vector4 left       = leftLen > 0.0f ? leftRaw / leftLen : leftRaw;
    Vector4 top        = topLen > 0.0f ? topRaw / topLen : topRaw;
    sceneFrame.Frustum = Vector4(left.x, -left.z, top.y, -top.z);

    sceneFrame.P00   = projectionMatrix[0][0];
    sceneFrame.P11   = projectionMatrix[1][1];
    sceneFrame.ZNear = k_zNear;
    sceneFrame.ZFar  = k_zFar;

    sceneFrame.VertexBuffer           = m_vertexStorageBuffer->GetDeviceAddress();
    sceneFrame.MeshletBuffer          = m_meshletBuffer->GetDeviceAddress();
    sceneFrame.MeshletDataBuffer      = m_meshletDataBuffer->GetDeviceAddress();
    sceneFrame.MeshDataBuffer         = m_meshBuffer->GetDeviceAddress();
    sceneFrame.MeshDrawBuffer         = m_meshDrawBuffer->GetDeviceAddress();
    sceneFrame.VisibleDrawIndexBuffer = m_visibleDrawIndexBuffer->GetDeviceAddress();
    sceneFrame.IndirectCommandBuffer  = m_meshTaskIndirectBuffer->GetDeviceAddress();
    sceneFrame.IndirectCountBuffer    = m_meshTaskIndirectCountBuffer->GetDeviceAddress();
    sceneFrame.ObjectVisBuffer        = m_objectVisBuffer->GetDeviceAddress();
    sceneFrame.MeshletVisBuffer       = m_meshletVisBuffer->GetDeviceAddress();
    sceneFrame.MaterialBuffer         = m_materialBuffer->GetDeviceAddress();

    std::vector<RenderBatch> renderBatches;
    renderBatches.emplace_back(); // single batch -- one pipeline pair for everyone

    // Per-frame material upload state (bindless materials). uploadMaterialBytes
    // holds packed MaterialData blobs (one Stride() bytes per unique Material).
    // materialIndexMap dedupes Material* references during collect so 10K
    // instances of one Material yield one entry. Both reset on mid-frame flush.
    std::vector<uint8_t>                    uploadMaterialBytes;
    std::unordered_map<Material*, uint32_t> materialIndexMap;
    auto&                                   matSchema = MaterialSchema::Default();
    // Texture-typed fields (e.g. AlbedoTexture) come into Pack() as
    // WRef<ITexture>; this lambda resolves each to its bindless slot via the
    // engine-level BindlessTextures manager. m_materialViews caches one
    // ITextureView per ITexture so the slot stays live (BindlessTextures
    // dedupes by view pointer; the cache makes that effective per Material).
    MaterialSchema::TextureResolver textureResolver = [this](const WRef<ITexture>& tex) -> uint32_t {
        if (!tex)
            return 0u; // default white at slot 0
        ITexture* key = tex.Get();
        auto      it  = m_materialViews.find(key);
        if (it == m_materialViews.end())
        {
            auto inserted = m_materialViews.emplace(key, ITextureView::Create(tex));
            it            = inserted.first;
        }
        return BindlessTextures::Get().Register(it->second.Get());
    };
    auto getOrAddMaterial = [&](const WRef<Material>& mat) -> uint32_t {
        Material* ptr = mat.Get();
        auto      it  = materialIndexMap.find(ptr);
        if (it != materialIndexMap.end())
            return it->second;
        const uint32_t stride = matSchema.Stride();
        uint32_t       idx    = static_cast<uint32_t>(uploadMaterialBytes.size() / stride);
        materialIndexMap[ptr] = idx;
        const size_t oldSize  = uploadMaterialBytes.size();
        uploadMaterialBytes.resize(oldSize + stride);
        matSchema.Pack(*ptr, uploadMaterialBytes.data() + oldSize, textureResolver);
        return idx;
    };

    // collect and batch meshes into meshlets
    world.ViewComponents<TransformComponent, MeshRendererComponent>([&](Entity                 entity,
                                                                        TransformComponent&    meshTransform,
                                                                        MeshRendererComponent& meshRenderer) {
        auto& mesh     = meshRenderer.Mesh;
        auto& material = meshRenderer.Material;
        if (!mesh || !material)
            return;

        uint32_t meshVertexCount  = static_cast<uint32_t>(mesh->GetVertices().size());
        uint32_t meshMeshletCount = static_cast<uint32_t>(mesh->GetMeshlets().size());

        if (renderBatches.back().MeshDraws.size() + 1 > MAX_MESH_DRAWS)
        {
            // Mid-frame flush -- still more draws coming, don't transition to present yet.
            FlushBatch(commandBuffer,
                       currentTarget,
                       /*transitionToPresent=*/false,
                       sceneFrame,
                       renderBatches,
                       uploadMaterialBytes);
            renderBatches.emplace_back();
            materialIndexMap.clear();
        }

        std::string assetKey = AssetManager::GetAssetKey(mesh);
        auto        meshKey  = MeshGPUUploadCache::BuildKey(mesh, assetKey, 0);

        uint32_t meshIndex = m_cachedMeshCount;
        if (!m_meshUploadCache.TryGet(meshKey, meshIndex))
        {
            uint32_t meshletDataSize = static_cast<uint32_t>(mesh->GetMeshletData().size());

            if (m_cachedVertexCount + meshVertexCount > MAX_VERTICES ||
                m_cachedMeshletCount + meshMeshletCount > MAX_MESHLETS ||
                m_cachedMeshletDataSize + meshletDataSize > MAX_MESHLET_DATA_SIZE / sizeof(uint32_t))
            {
                // Mid-frame flush due to upload cache pressure; not the final pass.
                FlushBatch(commandBuffer,
                           currentTarget,
                           /*transitionToPresent=*/false,
                           sceneFrame,
                           renderBatches,
                           uploadMaterialBytes);
                renderBatches.emplace_back();
                materialIndexMap.clear();
                ResetMeshUploadCache();
            }

            MeshData meshData{};
            meshData.BoundingCenter  = mesh->GetCenter();
            meshData.BoundingRadius  = mesh->GetBoundingRadius();
            meshData.VertexOffset    = m_cachedVertexCount;
            meshData.MeshletOffset   = m_cachedMeshletCount;
            meshData.MeshletCount    = meshMeshletCount;
            meshData.MeshletDataBase = m_cachedMeshletDataSize;
            m_meshBuffer->UpdateData(&meshData, sizeof(MeshData), static_cast<uint64_t>(meshIndex) * sizeof(MeshData));
            if (!mesh->GetVertices().empty())
            {
                m_vertexStorageBuffer->UpdateData(mesh->GetVertices().data(),
                                                  mesh->GetVertices().size() * sizeof(VertexData),
                                                  static_cast<uint64_t>(meshData.VertexOffset) * sizeof(VertexData));
            }
            if (!mesh->GetMeshlets().empty())
            {
                m_meshletBuffer->UpdateData(mesh->GetMeshlets().data(),
                                            mesh->GetMeshlets().size() * sizeof(MeshletData),
                                            static_cast<uint64_t>(meshData.MeshletOffset) * sizeof(MeshletData));
            }
            if (!mesh->GetMeshletData().empty())
            {
                m_meshletDataBuffer->UpdateData(mesh->GetMeshletData().data(),
                                                mesh->GetMeshletData().size() * sizeof(uint32_t),
                                                static_cast<uint64_t>(meshData.MeshletDataBase) * sizeof(uint32_t));
            }

            m_cachedVertexCount += meshVertexCount;
            m_cachedMeshletCount += meshMeshletCount;
            m_cachedMeshletDataSize += meshletDataSize;
            m_cachedMeshCount += 1;

            // Parallel array indexed by meshIndex -- needed by FlushBatch's
            // MeshletVisOffset prefix sum. Every newly-allocated meshIndex
            // here gets the matching MeshletCount appended; old indices stay valid
            // (their MeshData is still resident in m_meshBuffer until the next
            // ResetMeshUploadCache, which only nukes the upload cache, not the
            // GPU-side MeshData slot).
            if (m_cachedMeshMeshletCounts.size() <= meshIndex)
            {
                m_cachedMeshMeshletCounts.resize(meshIndex + 1, 0u);
            }
            m_cachedMeshMeshletCounts[meshIndex] = meshMeshletCount;

            m_meshUploadCache.Upsert(meshKey, meshIndex);
        }

        MeshDraw draw{};
        draw.Position      = meshTransform.Transform.Position;
        draw.Scale         = meshTransform.Transform.Scale;
        draw.Orientation   = Vector4(meshTransform.Transform.Rotation.x,
                                     meshTransform.Transform.Rotation.y,
                                     meshTransform.Transform.Rotation.z,
                                     meshTransform.Transform.Rotation.w);
        draw.MeshIndex     = meshIndex;
        draw.MaterialIndex = getOrAddMaterial(material);
        renderBatches.back().MeshDraws.push_back(draw);
    });

    FlushBatch(commandBuffer, currentTarget, transitionToPresent, sceneFrame, renderBatches, uploadMaterialBytes);
}

void ForwardRenderSystem::FlushBatch(ICommandBuffer*           commandBuffer,
                                     ITextureView*             colorView,
                                     bool                      transitionToPresent,
                                     SceneFrame&               sceneFrame,
                                     std::vector<RenderBatch>& renderBatches,
                                     std::vector<uint8_t>&     uploadMaterialBytes)
{
    commandBuffer->Wait();
    if (renderBatches.empty())
        return;

    auto           swapChain = Application::GetInstance().GetSwapChain();
    const uint32_t frameSlot = swapChain->GetCurrentFrameIndex();
    m_frameArena.BeginFrame(frameSlot);

    const uint64_t sceneFrameAddr = m_frameArena.Push(&sceneFrame, sizeof(SceneFrame));

    std::vector<MeshDraw> uploadMeshDraws;

    for (auto& batch : renderBatches)
    {
        if (batch.MeshDraws.empty())
            continue;

        batch.DrawBase            = static_cast<uint32_t>(uploadMeshDraws.size());
        batch.DrawCount           = static_cast<uint32_t>(batch.MeshDraws.size());
        batch.IndirectOffsetBytes = batch.DrawBase * sizeof(uint32_t) * 3;

        uploadMeshDraws.insert(uploadMeshDraws.end(), batch.MeshDraws.begin(), batch.MeshDraws.end());
    }

    // Prefix sum: each MeshDraw's MeshletVisOffset is the cumulative
    // MeshletCount of all preceding draws. globalMeshletId in the task shader is
    // (draw.MeshletVisOffset + miLocal), so two instances of the same Cube
    // get distinct visibility slots. Capped at MAX_MESHLET_VIS_COUNT --
    // overflow ABORTS the frame: the task shader writes mvBuf via BDA which has
    // no descriptor bounds check, so an out-of-range visIdx would corrupt
    // adjacent device memory (fence/semaphore state of other queues, in the
    // worst case) and we've seen that take down the whole Vulkan device.
    {
        uint32_t cumMeshletCount = 0;
        for (auto& draw : uploadMeshDraws)
        {
            const uint32_t meshletCount =
                draw.MeshIndex < m_cachedMeshMeshletCounts.size() ? m_cachedMeshMeshletCounts[draw.MeshIndex] : 0u;
            draw.MeshletVisOffset = cumMeshletCount;
            cumMeshletCount += meshletCount;
        }
        if (cumMeshletCount > MAX_MESHLET_VIS_COUNT)
        {
            YG_CORE_ERROR("MeshletVis cap exceeded: needed {0}, cap {1}. Skipping frame to "
                          "avoid out-of-bounds BDA writes from the task shader.",
                          cumMeshletCount,
                          MAX_MESHLET_VIS_COUNT);
            renderBatches.clear();
            uploadMaterialBytes.clear();
            return;
        }
    }

    m_meshDrawBuffer->UpdateData(
        uploadMeshDraws.data(), static_cast<uint32_t>(uploadMeshDraws.size() * sizeof(MeshDraw)), 0);

    // Bindless materials: uploadMaterialBytes is a packed run of Stride()-byte
    // MaterialData blobs, one per unique Material referenced this frame. Empty
    // when no MeshRenderers were drawn. Cap as a hard guard since indices are
    // unbounded BDA reads on the GPU side.
    if (!uploadMaterialBytes.empty())
    {
        const uint32_t stride        = MaterialSchema::Default().Stride();
        const size_t   materialCount = stride > 0 ? uploadMaterialBytes.size() / stride : 0;
        if (materialCount > MAX_MATERIALS)
        {
            YG_CORE_ERROR("Material cap exceeded: needed {0}, cap {1}. Skipping frame.", materialCount, MAX_MATERIALS);
            renderBatches.clear();
            uploadMaterialBytes.clear();
            return;
        }
        m_materialBuffer->UpdateData(uploadMaterialBytes.data(), static_cast<uint32_t>(uploadMaterialBytes.size()), 0);
    }

    uint32_t totalDrawCount  = static_cast<uint32_t>(uploadMeshDraws.size());
    uint32_t totalBatchCount = static_cast<uint32_t>(renderBatches.size());

    const uint32_t kLateRegionBase = static_cast<uint32_t>(MAX_MESH_DRAWS);

    const uint32_t targetWidth  = colorView->GetTexture()->GetWidth();
    const uint32_t targetHeight = colorView->GetTexture()->GetHeight();

    commandBuffer->Begin();

    // The depth pyramid is fully bindless: its mip slots auto-(re)register on
    // every Resize -- nothing to wire up here beyond that.
    m_depthPyramid.Resize(m_depthTexture->GetWidth(), m_depthTexture->GetHeight());
    const bool pyramidValid = m_depthPyramid.IsValid();

    // Zero the per-batch counters for BOTH EARLY (slots [0..B)) and LATE (slots [B..2B)).
    {
        std::vector<uint32_t> zeroCounts(2 * totalBatchCount, 0);
        m_meshTaskIndirectCountBuffer->UpdateData(
            zeroCounts.data(), static_cast<uint32_t>(zeroCounts.size() * sizeof(uint32_t)), 0);
    }

    if (pyramidValid)
    {
        m_depthPyramid.EnsureInitialLayout(commandBuffer);
    }

    // ================ EARLY cull dispatch ================
    commandBuffer->SetPipeline(m_cullPipelineEarly.Get());
    commandBuffer->SetShaderResourceBinding(BindlessTextures::Get().GetSRB());

    CullPush pcCull{};
    pcCull.SceneFrameAddr = sceneFrameAddr;
    pcCull.PyramidSlot    = 0; // unused by EARLY (no Hi-Z probing)

    for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
    {
        auto& batch = renderBatches[batchIndex];
        if (batch.DrawCount == 0)
            continue;

        pcCull.DrawBase   = batch.DrawBase;
        pcCull.DrawCount  = batch.DrawCount;
        pcCull.OutputBase = batch.DrawBase;
        pcCull.CountIndex = static_cast<uint32_t>(batchIndex);

        commandBuffer->SetPushConstants(m_cullPipelineEarly.Get(), ShaderStage::Compute, 0, sizeof(CullPush), &pcCull);

        uint32_t dispatchX = (batch.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
        commandBuffer->Dispatch(dispatchX, 1, 1);
    }

    // Indirect buffer EARLY region: compute write -> task/mesh/indirect read.
    if (totalDrawCount > 0)
    {
        BarrierDesc barrierDesc{};
        barrierDesc.Buffer      = m_meshTaskIndirectBuffer.Get();
        barrierDesc.BeforeState = ResourceState::UnorderedAccess;
        barrierDesc.AfterState =
            ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource;
        commandBuffer->Barrier(barrierDesc);
    }

    // ================ EARLY rendering pass ================
    // Clears color + depth -- this is the "draws visible last frame" pass and
    // establishes the new frame's color + depth baseline. Both attachments are
    // overwritten (LoadOp::Clear), so Discard their prior contents.
    commandBuffer->Barrier(BarrierDesc{
        .TextureView = colorView,
        .BeforeState = ResourceState::Undefined,
        .AfterState  = ResourceState::ColorAttachment,
    });
    commandBuffer->Barrier(BarrierDesc{
        .TextureView = m_depthView.Get(),
        .BeforeState = ResourceState::Undefined,
        .AfterState  = ResourceState::DepthWrite,
    });
    {
        RenderingDesc rdesc{};
        rdesc.Width   = targetWidth;
        rdesc.Height  = targetHeight;
        rdesc.Samples = SampleCountFlagBits::Count1;
        RenderingAttachment color{};
        color.View              = colorView;
        color.LoadAction        = LoadOp::Clear;
        color.StoreAction       = StoreOp::Store;
        color.ClearVal.Color[0] = 0.1f;
        color.ClearVal.Color[1] = 0.1f;
        color.ClearVal.Color[2] = 0.1f;
        color.ClearVal.Color[3] = 1.0f;
        rdesc.ColorAttachments.push_back(color);
        rdesc.DepthAttachment.View                          = m_depthView.Get();
        rdesc.DepthAttachment.LoadAction                    = LoadOp::Clear;
        rdesc.DepthAttachment.StoreAction                   = StoreOp::Store;
        rdesc.DepthAttachment.ClearVal.DepthStencil.Depth   = 1.0f;
        rdesc.DepthAttachment.ClearVal.DepthStencil.Stencil = 0;
        commandBuffer->BeginRendering(rdesc);
    }
    commandBuffer->SetPipeline(m_meshletEarlyPipeline.Get());
    commandBuffer->SetShaderResourceBinding(BindlessTextures::Get().GetSRB());
    commandBuffer->SetViewport({ 0, 0, (float)targetWidth, (float)targetHeight });
    commandBuffer->SetScissor({ 0, 0, targetWidth, targetHeight });

    for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
    {
        auto& batch = renderBatches[batchIndex];
        if (batch.DrawCount == 0)
            continue;

        ScenePush pcScene{};
        pcScene.SceneFrameAddr = sceneFrameAddr;
        pcScene.DrawBase       = batch.DrawBase;
        pcScene.PyramidSlot    = 0; // unused by EARLY task
        commandBuffer->SetPushConstants(m_meshletEarlyPipeline.Get(),
                                        ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                                        0,
                                        sizeof(ScenePush),
                                        &pcScene);
        commandBuffer->DrawMeshTasksIndirectCount(m_meshTaskIndirectBuffer.Get(),
                                                  batch.IndirectOffsetBytes,
                                                  m_meshTaskIndirectCountBuffer.Get(),
                                                  static_cast<uint32_t>(batchIndex * sizeof(uint32_t)),
                                                  batch.DrawCount,
                                                  sizeof(uint32_t) * 3);
    }

    commandBuffer->EndRendering();

    // ================ Depth Pyramid (Hi-Z) build ================
    const bool pyramidReady = m_depthPyramid.IsValid() && m_depthReducePipeline;
    if (pyramidReady)
    {
        // Depth attachment is currently in DEPTH_ATTACHMENT_OPTIMAL (set by
        // BeginRendering's transition). DepthReduce.comp samples it as a regular
        // texture, so flip to DepthRead before sampling and back after.
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = m_depthView.Get(),
            .BeforeState = ResourceState::DepthWrite,
            .AfterState  = ResourceState::ComputeShaderResource,
        });

        m_depthPyramid.Build(commandBuffer, m_depthReducePipeline.Get(), m_depthView.Get());

        commandBuffer->Barrier(BarrierDesc{
            .TextureView = m_depthView.Get(),
            .BeforeState = ResourceState::ComputeShaderResource,
            .AfterState  = ResourceState::DepthWrite,
        });
    }

    // ================ LATE cull dispatch ================
    const bool runLatePass = pyramidReady && totalDrawCount > 0;
    if (runLatePass)
    {
        commandBuffer->Barrier(BarrierDesc{
            .Buffer      = m_objectVisBuffer.Get(),
            .BeforeState = ResourceState::UnorderedAccess,
            .AfterState  = ResourceState::UnorderedAccess,
        });

        // Pyramid was just written by the reduce dispatches. Both LATE cull
        // (compute sampler2D read) and LATE meshlet task (task sampler2D read)
        // read it via SAMPLED, not via storage. UnorderedAccess->UnorderedAccess
        // alone only flushes STORAGE_READ|STORAGE_WRITE on the dst side, which
        // does NOT cover SHADER_SAMPLED_READ_BIT -- the consumer reads can race
        // against still-in-flight storage writes from the last reduce dispatch
        // -> wrong Hi-Z verdict -> meshlets and objects flicker. OR-ing in
        // {Compute,Task,Mesh}ShaderResource adds SHADER_READ_BIT to dstAccess
        // (covers sampled reads); UnorderedAccess in BOTH states keeps the
        // image layout in GENERAL (the state->layout function picks
        // UnorderedAccess first), so the next frame's pyramid build can keep
        // writing via image2D without an extra layout transition.
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = m_depthPyramid.GetTextureView(),
            .BeforeState = ResourceState::UnorderedAccess,
            .AfterState  = ResourceState::UnorderedAccess | ResourceState::ComputeShaderResource |
                ResourceState::TaskShaderResource | ResourceState::MeshShaderResource,
        });

        commandBuffer->SetPipeline(m_cullPipelineLate.Get());
        commandBuffer->SetShaderResourceBinding(BindlessTextures::Get().GetSRB());

        const uint32_t pyramidSlot = m_depthPyramid.GetPyramidSampledSlot();

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (batch.DrawCount == 0)
                continue;

            pcCull.DrawBase    = batch.DrawBase;
            pcCull.DrawCount   = batch.DrawCount;
            pcCull.OutputBase  = kLateRegionBase + batch.DrawBase;
            pcCull.CountIndex  = totalBatchCount + static_cast<uint32_t>(batchIndex);
            pcCull.PyramidSlot = pyramidSlot;

            commandBuffer->SetPushConstants(
                m_cullPipelineLate.Get(), ShaderStage::Compute, 0, sizeof(CullPush), &pcCull);

            uint32_t dispatchX = (batch.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
            commandBuffer->Dispatch(dispatchX, 1, 1);
        }

        commandBuffer->Barrier(BarrierDesc{
            .Buffer      = m_meshTaskIndirectBuffer.Get(),
            .BeforeState = ResourceState::UnorderedAccess,
            .AfterState =
                ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource,
        });

        // ================ LATE rendering pass ================
        // LoadOp::Load preserves both color (from EARLY) and depth (from EARLY +
        // pyramid build roundtrip) so the late draws composite on top.
        // Color: EndRendering left the image in ColorAttachmentOptimal but
        //   issued no sync to the LATE pass's attachment loads. A self-
        //   transition Barrier (ColorAttachment->ColorAttachment) carries the
        //   needed memory dependency without changing the layout.
        // Depth: the pyramid roundtrip's last barrier (ComputeShaderResource
        //   -> DepthWrite) already drained the compute writes, so no extra
        //   barrier here.
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = colorView,
            .BeforeState = ResourceState::ColorAttachment,
            .AfterState  = ResourceState::ColorAttachment,
        });
        {
            RenderingDesc rdesc{};
            rdesc.Width   = targetWidth;
            rdesc.Height  = targetHeight;
            rdesc.Samples = SampleCountFlagBits::Count1;
            RenderingAttachment color{};
            color.View        = colorView;
            color.LoadAction  = LoadOp::Load;
            color.StoreAction = StoreOp::Store;
            rdesc.ColorAttachments.push_back(color);
            rdesc.DepthAttachment.View        = m_depthView.Get();
            rdesc.DepthAttachment.LoadAction  = LoadOp::Load;
            rdesc.DepthAttachment.StoreAction = StoreOp::Store;
            commandBuffer->BeginRendering(rdesc);
        }
        commandBuffer->SetPipeline(m_meshletLatePipeline.Get());
        commandBuffer->SetShaderResourceBinding(BindlessTextures::Get().GetSRB());
        commandBuffer->SetViewport({ 0, 0, (float)targetWidth, (float)targetHeight });
        commandBuffer->SetScissor({ 0, 0, targetWidth, targetHeight });

        for (size_t batchIndex = 0; batchIndex < renderBatches.size(); ++batchIndex)
        {
            auto& batch = renderBatches[batchIndex];
            if (batch.DrawCount == 0)
                continue;

            ScenePush pcScene{};
            pcScene.SceneFrameAddr = sceneFrameAddr;
            pcScene.DrawBase       = kLateRegionBase + batch.DrawBase;
            pcScene.PyramidSlot    = pyramidSlot;
            commandBuffer->SetPushConstants(m_meshletLatePipeline.Get(),
                                            ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                                            0,
                                            sizeof(ScenePush),
                                            &pcScene);

            const uint32_t lateIndirectOffset =
                static_cast<uint32_t>(kLateRegionBase + batch.DrawBase) * sizeof(uint32_t) * 3;
            const uint32_t lateCountOffset = static_cast<uint32_t>((totalBatchCount + batchIndex) * sizeof(uint32_t));

            commandBuffer->DrawMeshTasksIndirectCount(m_meshTaskIndirectBuffer.Get(),
                                                      lateIndirectOffset,
                                                      m_meshTaskIndirectCountBuffer.Get(),
                                                      lateCountOffset,
                                                      batch.DrawCount,
                                                      sizeof(uint32_t) * 3);
        }

        commandBuffer->EndRendering();
    }

    // Final cmd-buffer step: when the caller is rendering directly to the
    // swapchain (Sandbox, no Editor compositing layer), our last write left the
    // image in COLOR_ATTACHMENT_OPTIMAL via EndRendering. vkQueuePresentKHR
    // requires PRESENT_SRC_KHR -- with dynamic rendering there's no implicit
    // VkRenderPass finalLayout to do it for us, so we transition explicitly here.
    if (transitionToPresent)
    {
        commandBuffer->Barrier(BarrierDesc{
            .TextureView = colorView,
            .BeforeState = ResourceState::ColorAttachment,
            .AfterState  = ResourceState::Present,
        });
    }

    commandBuffer->End();
    commandBuffer->Submit();

    renderBatches.clear();
    uploadMaterialBytes.clear();
}

void ForwardRenderSystem::EnsureDepthTexture(uint32_t width, uint32_t height)
{
    if (m_depthTexture && m_depthTexture->GetWidth() == width && m_depthTexture->GetHeight() == height)
        return;

    TextureDesc desc{};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = 1;
    desc.Format     = m_depthFormat;
    desc.Usage      = ITexture::Usage::DepthStencil;
    desc.NumSamples = SampleCountFlagBits::Count1;
    m_depthTexture  = ResourceManager::CreateResource<ITexture>(desc);
    m_depthView     = ResourceManager::CreateResource<ITextureView>(m_depthTexture);
}

} // namespace Yogi
