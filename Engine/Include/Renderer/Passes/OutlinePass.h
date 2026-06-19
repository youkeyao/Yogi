#pragma once

#include "Renderer/Passes/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITextureView.h"
#include "Renderer/PipelineRegistry.h"

namespace Yogi
{

struct DrawBucket
{
    std::string ShaderKey;
    uint32_t    DrawBase;
    uint32_t    DrawCount;
    uint64_t    MaterialBufferAddr;
};

class YG_API OutlinePass : public RenderPass
{
public:
    OutlinePass()           = default;
    ~OutlinePass() override = default;

    void SetIndirectBuffers(WRef<IBuffer> indirectCommandBuffer, WRef<IBuffer> indirectCountBuffer)
    {
        m_indirectCmd   = indirectCommandBuffer;
        m_indirectCount = indirectCountBuffer;
    }

    void Initialize() override;
    void Shutdown() override;

    // -- RenderPass overrides (pipeline ownership) ---------------
    SpecializedPipelineBuilder GetPipelineBuilder() override;
    std::string                GetPassName() const override { return "Outline"; }

    // -- Execution ------------------------------------------------
    void Execute(ICommandBuffer*                 cmd,
                 uint64_t                        sceneFrameAddr,
                 const SpecializedPipelinePair&  pipelinePair,
                 const std::vector<DrawBucket>&  buckets,
                 const std::vector<std::string>& bucketTypeNames);

private:
    WRef<IBuffer> m_indirectCmd   = nullptr;
    WRef<IBuffer> m_indirectCount = nullptr;
};

} // namespace Yogi
