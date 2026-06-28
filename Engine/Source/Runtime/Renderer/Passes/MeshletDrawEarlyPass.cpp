#include "Renderer/Passes/MeshletDrawEarlyPass.h"
#include "Renderer/BindlessTextureManager.h"
#include "Renderer/RenderGraph.h"
#include "Core/Application.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

namespace
{
std::string ResolveMaterialTypeName(const std::string& shaderKey)
{
    std::string materialTypeName = "StandardMaterial";
    size_t      pos              = shaderKey.find_last_of('/');
    if (pos != std::string::npos)
    {
        std::string stem = shaderKey.substr(pos + 1);
        size_t      dot  = stem.find_last_of('.');
        if (dot != std::string::npos)
            stem = stem.substr(0, dot);
        materialTypeName = stem + "Material";
    }
    return materialTypeName;
}
} // namespace

WRef<IPipeline> MeshletDrawEarlyPass::BuildPipeline(const std::string& shaderKey)
{
    const std::string materialTypeName = ResolveMaterialTypeName(shaderKey);
    const Format      colorFormat      = Application::GetInstance().GetSwapChain()->GetColorFormat();

    WRef<ShaderDesc> task = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Passes/Meshlet.as.slang");
    WRef<ShaderDesc> mesh = AssetManager::AcquireAsset<ShaderDesc>(
        "EngineAssets/Shaders/Passes/Meshlet.ms.slang::__SPECIALIZE_M=" + materialTypeName);
    WRef<ShaderDesc> frag = AssetManager::AcquireAsset<ShaderDesc>(
        "EngineAssets/Shaders/Passes/Meshlet.fs.slang::__SPECIALIZE_M=" + materialTypeName);

    if (!task.Get() || !mesh.Get() || !frag.Get())
    {
        YG_CORE_ERROR(
            "MeshletDrawEarlyPass: shader load failed for type '{0}' (shaderKey='{1}')", materialTypeName, shaderKey);
        return nullptr;
    }

    PipelineDesc desc{};
    desc.ResourceBinding         = BindlessTextureManager::GetSRB();
    desc.PushConstantRanges      = { PushConstantRange{ ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                                                        0,
                                                        static_cast<uint32_t>(sizeof(MeshletDrawPush)) } };
    desc.ColorTargets            = { ColorTargetDesc{ .Format = colorFormat } };
    desc.DepthFormat             = Format::D24_UNORM_S8_UINT;
    desc.Samples                 = SampleCountFlagBits::Count1;
    desc.Topology                = PrimitiveTopology::TriangleList;
    desc.Stencil.Enable          = true;
    desc.Stencil.Reference       = 1;
    desc.Stencil.Front.CompareFn = CompareOp::Always;
    desc.Stencil.Front.PassOp    = StencilOp::Replace;
    desc.Stencil.Back            = desc.Stencil.Front;

    desc.Shaders = { task.Get(), mesh.Get(), frag.Get() };
    return ResourceManager::AcquireSharedResource<IPipeline>(desc);
}

WRef<IPipeline> MeshletDrawEarlyPass::AcquirePipeline(const std::string& shaderKey)
{
    if (shaderKey.empty())
    {
        YG_CORE_WARN("MeshletDrawEarlyPass::AcquirePipeline called with empty shaderKey");
        return nullptr;
    }

    auto it = m_pipelineCache.find(shaderKey);
    if (it != m_pipelineCache.end())
        return it->second;

    WRef<IPipeline> pipeline = BuildPipeline(shaderKey);
    if (!pipeline)
        return nullptr;

    m_pipelineCache[shaderKey] = pipeline;
    return pipeline;
}

void MeshletDrawEarlyPass::Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder)
{
    m_indirectCmd            = graph.GetBuffer("ObjectCull.IndirectCommand");
    m_indirectCount          = graph.GetBuffer("ObjectCull.IndirectCount");
    m_visibleDrawIndexBuffer = graph.GetBuffer("ObjectCull.VisibleDrawIndex");
    m_meshletVis[0]          = graph.GetBuffer("ObjectCull.MeshletVis0");
    m_meshletVis[1]          = graph.GetBuffer("ObjectCull.MeshletVis1");
    m_vertexBuffer           = graph.GetBuffer("Geometry.Vertex");
    m_meshletBuffer          = graph.GetBuffer("Geometry.Meshlet");
    m_meshletDataBuffer      = graph.GetBuffer("Geometry.MeshletData");
    m_meshDataBuffer         = graph.GetBuffer("Geometry.MeshData");
    m_meshDrawBuffer         = graph.GetBuffer("Geometry.MeshDraw");

    m_readIdx  = 1u - m_readIdx;
    m_writeIdx = 1u - m_readIdx;
    const ResourceState meshletState =
        ResourceState::UnorderedAccess | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource;

    builder.UseBuffer(m_indirectCmd.Get(), ResourceState::IndirectArg | ResourceState::TaskShaderResource);
    builder.UseBuffer(m_indirectCount.Get(), ResourceState::IndirectArg);
    builder.UseBuffer(m_visibleDrawIndexBuffer.Get(), ResourceState::TaskShaderResource);
    builder.UseBuffer(m_meshletVis[m_readIdx].Get(), ResourceState::TaskShaderResource);
    builder.UseBuffer(m_meshletVis[m_writeIdx].Get(), meshletState);
    builder.UseBuffer(m_vertexBuffer.Get(), ResourceState::MeshShaderResource);
    builder.UseBuffer(m_meshletBuffer.Get(), ResourceState::TaskShaderResource | ResourceState::MeshShaderResource);
    builder.UseBuffer(m_meshletDataBuffer.Get(), ResourceState::MeshShaderResource);
    builder.UseBuffer(m_meshDataBuffer.Get(), ResourceState::TaskShaderResource | ResourceState::MeshShaderResource);
    builder.UseBuffer(m_meshDrawBuffer.Get(), ResourceState::TaskShaderResource | ResourceState::MeshShaderResource);
    builder.UseTexture(context.CurrentTarget, ResourceState::ColorAttachment);
    builder.UseTexture(context.DepthView, ResourceState::DepthWrite);
}

void MeshletDrawEarlyPass::Execute(RenderGraphContext& context)
{
    if (!context.Buckets || !context.CurrentTarget || !context.DepthView)
        return;

    RenderingDesc rdesc{};
    rdesc.Width   = context.TargetWidth;
    rdesc.Height  = context.TargetHeight;
    rdesc.Samples = SampleCountFlagBits::Count1;

    RenderingAttachment color{};
    color.View              = context.CurrentTarget;
    color.LoadAction        = LoadOp::Clear;
    color.StoreAction       = StoreOp::Store;
    color.ClearVal.Color[0] = 0.1f;
    color.ClearVal.Color[1] = 0.1f;
    color.ClearVal.Color[2] = 0.1f;
    color.ClearVal.Color[3] = 1.0f;
    rdesc.ColorAttachments.push_back(color);

    rdesc.DepthAttachment.View                          = context.DepthView;
    rdesc.DepthAttachment.LoadAction                    = LoadOp::Clear;
    rdesc.DepthAttachment.StoreAction                   = StoreOp::Store;
    rdesc.DepthAttachment.ClearVal.DepthStencil.Depth   = 1.0f;
    rdesc.DepthAttachment.ClearVal.DepthStencil.Stencil = 0;

    ICommandBuffer* cmd = context.CommandBuffer;
    cmd->BeginRendering(rdesc);
    cmd->SetViewport({ 0, 0, static_cast<float>(context.TargetWidth), static_cast<float>(context.TargetHeight) });
    cmd->SetScissor({ 0, 0, context.TargetWidth, context.TargetHeight });
    for (size_t bi = 0; bi < context.Buckets->size(); ++bi)
    {
        const DrawBucket& b = (*context.Buckets)[bi];
        ExecuteBucket(cmd, context.SceneFrameAddr, b, static_cast<uint32_t>(bi), m_readIdx, m_writeIdx);
    }
    cmd->EndRendering();
}

void MeshletDrawEarlyPass::ExecuteBucket(ICommandBuffer*   cmd,
                                         uint64_t          sceneFrameAddr,
                                         const DrawBucket& bucket,
                                         uint32_t          bucketIndex,
                                         uint32_t          readIdx,
                                         uint32_t          writeIdx)
{
    if (bucket.DrawCount == 0 || !m_indirectCmd || !m_indirectCount)
        return;

    WRef<IPipeline> pipeline = AcquirePipeline(bucket.ShaderKey);
    if (!pipeline)
        return;

    cmd->BeginDebugLabel("MeshletDrawEarly::Execute");
    cmd->SetPipeline(pipeline.Get());
    cmd->SetShaderResourceBinding(BindlessTextureManager::GetSRB());

    MeshletDrawPush pcScene{};
    pcScene.SceneFrameAddr         = sceneFrameAddr;
    pcScene.MaterialBufferAddr     = bucket.MaterialBufferAddr;
    pcScene.VisibleDrawIndexBuffer = m_visibleDrawIndexBuffer->GetDeviceAddress();
    pcScene.MeshletVisBufferRead   = m_meshletVis[readIdx]->GetDeviceAddress();
    pcScene.MeshletVisBufferWrite  = m_meshletVis[writeIdx]->GetDeviceAddress();
    pcScene.VertexBuffer           = m_vertexBuffer->GetDeviceAddress();
    pcScene.MeshletBuffer          = m_meshletBuffer->GetDeviceAddress();
    pcScene.MeshletDataBuffer      = m_meshletDataBuffer->GetDeviceAddress();
    pcScene.MeshDataBuffer         = m_meshDataBuffer->GetDeviceAddress();
    pcScene.MeshDrawBuffer         = m_meshDrawBuffer->GetDeviceAddress();
    pcScene.DrawBase               = bucket.DrawBase;
    pcScene.PyramidSlot            = 0;
    cmd->SetPushConstants(pipeline.Get(),
                          ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                          0,
                          sizeof(MeshletDrawPush),
                          &pcScene);

    const uint32_t indirectOffsetBytes = bucket.DrawBase * sizeof(uint32_t) * 3;
    const uint32_t countOffsetBytes    = bucketIndex * sizeof(uint32_t);
    cmd->DrawMeshTasksIndirectCount(m_indirectCmd.Get(),
                                    indirectOffsetBytes,
                                    m_indirectCount.Get(),
                                    countOffsetBytes,
                                    bucket.DrawCount,
                                    sizeof(uint32_t) * 3);

    cmd->EndDebugLabel();
}

} // namespace Yogi
