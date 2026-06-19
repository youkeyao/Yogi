#include "Renderer/Passes/OutlinePass.h"
#include "Renderer/BindlessTextureManager.h"
#include "Core/Application.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

SpecializedPipelineBuilder OutlinePass::GetPipelineBuilder()
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

        const std::string asKey = "EngineAssets/Shaders/Passes/Meshlet.as.slang::LATE=1";
        const std::string msKey = "EngineAssets/Shaders/Passes/Outline.ms.slang::__SPECIALIZE_M=" + materialTypeName;
        const std::string fsKey = "EngineAssets/Shaders/Passes/Outline.fs.slang::__SPECIALIZE_M=" + materialTypeName;

        WRef<ShaderDesc> task = AssetManager::AcquireAsset<ShaderDesc>(asKey);
        WRef<ShaderDesc> mesh = AssetManager::AcquireAsset<ShaderDesc>(msKey);
        WRef<ShaderDesc> frag = AssetManager::AcquireAsset<ShaderDesc>(fsKey);
        if (!task.Get() || !mesh.Get() || !frag.Get())
        {
            YG_CORE_ERROR("OutlinePass builder: shader load failed for type '{0}' (shaderKey='{1}')",
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
        desc.Cull               = CullMode::Front;

        SpecializedPipelinePair pair;
        desc.Shaders = { task.Get(), mesh.Get(), frag.Get() };
        pair.Late    = ResourceManager::AcquireSharedResource<IPipeline>(desc);
        pair.Early   = pair.Late; // Outline only uses Late
        return pair;
    };
}

void OutlinePass::Initialize()
{
    // Pipeline building is now handled by PipelineRegistry via
    // the builder returned from GetPipelineBuilder().
}

void OutlinePass::Shutdown()
{
    m_indirectCmd   = nullptr;
    m_indirectCount = nullptr;
}

void OutlinePass::Execute(ICommandBuffer*                 cmd,
                          uint64_t                        sceneFrameAddr,
                          const SpecializedPipelinePair&  pipelinePair,
                          const std::vector<DrawBucket>&  buckets,
                          const std::vector<std::string>& bucketTypeNames)
{
    if (buckets.empty() || !m_indirectCmd || !m_indirectCount)
        return;
    cmd->BeginDebugLabel("OutlinePass::Execute");

    if (!pipelinePair.Late)
    {
        YG_CORE_WARN("OutlinePass: pipeline pair is invalid, Execute skipped");
        return;
    }

    cmd->SetPipeline(pipelinePair.Late.Get());
    cmd->SetShaderResourceBinding(BindlessTextureManager::GetSRB());

    for (size_t bi = 0; bi < buckets.size(); ++bi)
    {
        const DrawBucket&  b        = buckets[bi];
        const std::string& typeName = bucketTypeNames[bi];

        ScenePush pcScene{};
        pcScene.SceneFrameAddr     = sceneFrameAddr;
        pcScene.MaterialBufferAddr = b.MaterialBufferAddr;
        pcScene.DrawBase           = b.DrawBase;
        pcScene.PyramidSlot        = 0; // outline doesn't sample HZB
        cmd->SetPushConstants(pipelinePair.Late.Get(),
                              ShaderStage::Task | ShaderStage::Mesh | ShaderStage::Fragment,
                              0,
                              sizeof(ScenePush),
                              &pcScene);

        const uint32_t indirectOffsetBytes = b.DrawBase * sizeof(uint32_t) * 3;
        const uint32_t countOffsetBytes    = static_cast<uint32_t>(bi) * sizeof(uint32_t);
        cmd->DrawMeshTasksIndirectCount(m_indirectCmd.Get(),
                                        indirectOffsetBytes,
                                        m_indirectCount.Get(),
                                        countOffsetBytes,
                                        b.DrawCount,
                                        sizeof(uint32_t) * 3);
    }

    cmd->EndDebugLabel();
}

} // namespace Yogi
