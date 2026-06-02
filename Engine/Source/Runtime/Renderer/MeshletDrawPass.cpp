#include "Renderer/MeshletDrawPass.h"
#include "Renderer/BindlessTextureManager.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

void MeshletDrawPass::Initialize()
{
    YG_CORE_ASSERT(m_indirectCmd && m_indirectCount,
                   "MeshletDrawPass: SetIndirectBuffers must be called before Initialize");
    YG_CORE_ASSERT(m_colorFormat != ITexture::Format::NONE,
                   "MeshletDrawPass: SetTargetColorFormat must be called before Initialize");

    WRef<ShaderDesc> taskShaderEarly = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Test.task");
    WRef<ShaderDesc> taskShaderLate  = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Test.task::LATE=1");
    WRef<ShaderDesc> meshShader      = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Test.mesh");
    WRef<ShaderDesc> fragmentShader  = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Test.frag");

    PipelineDesc desc{};
    desc.ResourceBinding    = BindlessTextureManager::GetSRB();
    desc.PushConstantRanges = { PushConstantRange{
        ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(ScenePush)) } };
    desc.ColorFormats       = { m_colorFormat };
    desc.DepthFormat        = ITexture::Format::D32_FLOAT;
    desc.Samples            = SampleCountFlagBits::Count1;
    desc.Topology           = PrimitiveTopology::TriangleList;

    desc.Shaders    = { taskShaderEarly.Get(), meshShader.Get(), fragmentShader.Get() };
    m_earlyPipeline = ResourceManager::AcquireSharedResource<IPipeline>(desc);

    desc.Shaders   = { taskShaderLate.Get(), meshShader.Get(), fragmentShader.Get() };
    m_latePipeline = ResourceManager::AcquireSharedResource<IPipeline>(desc);
}

void MeshletDrawPass::Shutdown()
{
    m_earlyPipeline = nullptr;
    m_latePipeline  = nullptr;
    m_indirectCmd   = nullptr;
    m_indirectCount = nullptr;
}

void MeshletDrawPass::ExecuteEarly(ICommandBuffer* cmd, uint64_t sceneFrameAddr, uint32_t drawBase, uint32_t drawCount)
{
    if (drawCount == 0 || !m_indirectCmd || !m_indirectCount)
        return;
    cmd->BeginDebugLabel("MeshletDraw::ExecuteEarly");

    cmd->SetPipeline(m_earlyPipeline.Get());
    cmd->SetShaderResourceBinding(BindlessTextureManager::GetSRB());

    ScenePush pcScene{};
    pcScene.SceneFrameAddr = sceneFrameAddr;
    pcScene.DrawBase       = drawBase;
    pcScene.PyramidSlot    = 0; // unused by EARLY task
    cmd->SetPushConstants(m_earlyPipeline.Get(),
                          ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                          0,
                          sizeof(ScenePush),
                          &pcScene);

    const uint32_t indirectOffsetBytes = drawBase * sizeof(uint32_t) * 3;
    cmd->DrawMeshTasksIndirectCount(m_indirectCmd.Get(),
                                    indirectOffsetBytes,
                                    m_indirectCount.Get(),
                                    /*countOffset*/ 0,
                                    drawCount,
                                    sizeof(uint32_t) * 3);

    cmd->EndDebugLabel();
}

void MeshletDrawPass::ExecuteLate(ICommandBuffer* cmd,
                                  uint64_t        sceneFrameAddr,
                                  uint32_t        drawBase,
                                  uint32_t        drawCount,
                                  uint32_t        pyramidSlot)
{
    if (drawCount == 0 || !m_indirectCmd || !m_indirectCount)
        return;
    cmd->BeginDebugLabel("MeshletDraw::ExecuteLate");

    cmd->SetPipeline(m_latePipeline.Get());
    cmd->SetShaderResourceBinding(BindlessTextureManager::GetSRB());

    ScenePush pcScene{};
    pcScene.SceneFrameAddr = sceneFrameAddr;
    pcScene.DrawBase       = drawBase;
    pcScene.PyramidSlot    = pyramidSlot;
    cmd->SetPushConstants(m_latePipeline.Get(),
                          ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                          0,
                          sizeof(ScenePush),
                          &pcScene);

    const uint32_t indirectOffsetBytes = drawBase * sizeof(uint32_t) * 3;
    cmd->DrawMeshTasksIndirectCount(m_indirectCmd.Get(),
                                    indirectOffsetBytes,
                                    m_indirectCount.Get(),
                                    /*countOffset*/ 0,
                                    drawCount,
                                    sizeof(uint32_t) * 3);

    cmd->EndDebugLabel();
}

} // namespace Yogi
