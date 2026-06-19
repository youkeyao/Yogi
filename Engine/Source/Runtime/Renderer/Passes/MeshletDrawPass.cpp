#include "Renderer/Passes/MeshletDrawPass.h"
#include "Renderer/BindlessTextureManager.h"
#include "Core/Application.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

SpecializedPipelineBuilder MeshletDrawPass::GetPipelineBuilder()
{
    return [](const PassKey& /*pass*/, const std::string& shaderKey) -> SpecializedPipelinePair {
        // Derive materialTypeName from shaderKey: "Standard.slang" -> "StandardMaterial"
        std::string materialTypeName = "StandardMaterial"; // default fallback
        size_t      pos              = shaderKey.find_last_of('/');
        if (pos != std::string::npos)
        {
            std::string stem = shaderKey.substr(pos + 1);
            // Remove ".slang" extension
            size_t dot = stem.find_last_of('.');
            if (dot != std::string::npos)
                stem = stem.substr(0, dot);
            materialTypeName = stem + "Material";
        }

        // Query swap-chain format at build time -- the pass owns this decision.
        const ITexture::Format colorFormat = Application::GetInstance().GetSwapChain()->GetColorFormat();

        const std::string asEarlyKey = "EngineAssets/Shaders/Passes/Meshlet.as.slang";
        const std::string asLateKey  = "EngineAssets/Shaders/Passes/Meshlet.as.slang::LATE=1";
        const std::string msKey = "EngineAssets/Shaders/Passes/Meshlet.ms.slang::__SPECIALIZE_M=" + materialTypeName;
        const std::string fsKey = "EngineAssets/Shaders/Passes/Meshlet.fs.slang::__SPECIALIZE_M=" + materialTypeName;

        WRef<ShaderDesc> taskEarly = AssetManager::AcquireAsset<ShaderDesc>(asEarlyKey);
        WRef<ShaderDesc> taskLate  = AssetManager::AcquireAsset<ShaderDesc>(asLateKey);
        WRef<ShaderDesc> mesh      = AssetManager::AcquireAsset<ShaderDesc>(msKey);
        WRef<ShaderDesc> frag      = AssetManager::AcquireAsset<ShaderDesc>(fsKey);

        if (!taskEarly.Get() || !taskLate.Get() || !mesh.Get() || !frag.Get())
        {
            YG_CORE_ERROR("MeshletDrawPass builder: shader load failed for type '{0}' (shaderKey='{1}')",
                          materialTypeName,
                          shaderKey);
            return {};
        }

        PipelineDesc desc{};
        desc.ResourceBinding    = BindlessTextureManager::GetSRB();
        desc.PushConstantRanges = { PushConstantRange{ ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                                                       0,
                                                       static_cast<uint32_t>(sizeof(ScenePush)) } };
        desc.ColorFormats       = { colorFormat };
        desc.DepthFormat        = ITexture::Format::D32_FLOAT;
        desc.Samples            = SampleCountFlagBits::Count1;
        desc.Topology           = PrimitiveTopology::TriangleList;

        SpecializedPipelinePair pair;
        desc.Shaders = { taskEarly.Get(), mesh.Get(), frag.Get() };
        pair.Early   = ResourceManager::AcquireSharedResource<IPipeline>(desc);
        desc.Shaders = { taskLate.Get(), mesh.Get(), frag.Get() };
        pair.Late    = ResourceManager::AcquireSharedResource<IPipeline>(desc);
        return pair;
    };
}

void MeshletDrawPass::Initialize()
{
    // Pipeline building is now handled by PipelineRegistry via
    // the builder returned from GetPipelineBuilder().
}

void MeshletDrawPass::Shutdown()
{
    m_indirectCmd   = nullptr;
    m_indirectCount = nullptr;
}

void MeshletDrawPass::ExecuteEarly(ICommandBuffer*                cmd,
                                   const SpecializedPipelinePair& pipelinePair,
                                   uint64_t                       sceneFrameAddr,
                                   uint32_t                       drawBase,
                                   uint32_t                       drawCount,
                                   uint64_t                       materialBufferAddr,
                                   const std::string&             shaderKey,
                                   uint32_t                       bucketIndex)
{
    if (drawCount == 0 || !m_indirectCmd || !m_indirectCount)
        return;

    if (!pipelinePair.Early)
    {
        YG_CORE_WARN("MeshletDraw::ExecuteEarly: no pipeline for shaderKey='{0}', bucket dropped", shaderKey);
        return;
    }

    cmd->BeginDebugLabel("MeshletDraw::ExecuteEarly");

    cmd->SetPipeline(pipelinePair.Early.Get());
    cmd->SetShaderResourceBinding(BindlessTextureManager::GetSRB());

    ScenePush pcScene{};
    pcScene.SceneFrameAddr     = sceneFrameAddr;
    pcScene.MaterialBufferAddr = materialBufferAddr;
    pcScene.DrawBase           = drawBase;
    pcScene.PyramidSlot        = 0; // unused by EARLY task
    cmd->SetPushConstants(pipelinePair.Early.Get(),
                          ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                          0,
                          sizeof(ScenePush),
                          &pcScene);

    const uint32_t indirectOffsetBytes = drawBase * sizeof(uint32_t) * 3;
    const uint32_t countOffsetBytes    = bucketIndex * sizeof(uint32_t);
    cmd->DrawMeshTasksIndirectCount(m_indirectCmd.Get(),
                                    indirectOffsetBytes,
                                    m_indirectCount.Get(),
                                    countOffsetBytes,
                                    drawCount,
                                    sizeof(uint32_t) * 3);

    cmd->EndDebugLabel();
}

void MeshletDrawPass::ExecuteLate(ICommandBuffer*                cmd,
                                  const SpecializedPipelinePair& pipelinePair,
                                  uint64_t                       sceneFrameAddr,
                                  uint32_t                       drawBase,
                                  uint32_t                       drawCount,
                                  uint32_t                       pyramidSlot,
                                  uint64_t                       materialBufferAddr,
                                  const std::string&             shaderKey,
                                  uint32_t                       bucketIndex)
{
    if (drawCount == 0 || !m_indirectCmd || !m_indirectCount)
        return;

    if (!pipelinePair.Late)
    {
        YG_CORE_WARN("MeshletDraw::ExecuteLate: no pipeline for shaderKey='{0}', bucket dropped", shaderKey);
        return;
    }

    cmd->BeginDebugLabel("MeshletDraw::ExecuteLate");

    cmd->SetPipeline(pipelinePair.Late.Get());
    cmd->SetShaderResourceBinding(BindlessTextureManager::GetSRB());

    ScenePush pcScene{};
    pcScene.SceneFrameAddr     = sceneFrameAddr;
    pcScene.DrawBase           = drawBase;
    pcScene.PyramidSlot        = pyramidSlot;
    pcScene.MaterialBufferAddr = materialBufferAddr;
    cmd->SetPushConstants(pipelinePair.Late.Get(),
                          ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                          0,
                          sizeof(ScenePush),
                          &pcScene);

    const uint32_t indirectOffsetBytes = drawBase * sizeof(uint32_t) * 3;
    const uint32_t countOffsetBytes    = bucketIndex * sizeof(uint32_t);
    cmd->DrawMeshTasksIndirectCount(m_indirectCmd.Get(),
                                    indirectOffsetBytes,
                                    m_indirectCount.Get(),
                                    countOffsetBytes,
                                    drawCount,
                                    sizeof(uint32_t) * 3);

    cmd->EndDebugLabel();
}

} // namespace Yogi
