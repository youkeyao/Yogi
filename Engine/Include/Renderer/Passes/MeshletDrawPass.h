#pragma once

#include "Renderer/Passes/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/ITextureView.h"
#include "Renderer/PipelineRegistry.h"

namespace Yogi
{

class YG_API MeshletDrawPass : public RenderPass
{
public:
    MeshletDrawPass()           = default;
    ~MeshletDrawPass() override = default;

    void SetIndirectBuffers(WRef<IBuffer> indirectCommandBuffer, WRef<IBuffer> indirectCountBuffer)
    {
        m_indirectCmd   = indirectCommandBuffer;
        m_indirectCount = indirectCountBuffer;
    }

    void Initialize() override;
    void Shutdown() override;

    // -- RenderPass overrides (pipeline ownership) ---------------
    SpecializedPipelineBuilder GetPipelineBuilder() override;
    std::string                GetPassName() const override { return "MeshletDraw"; }

    // -- Execution -----------------------------------------------
    void ExecuteEarly(ICommandBuffer*                cmd,
                      const SpecializedPipelinePair& pipelinePair,
                      uint64_t                       sceneFrameAddr,
                      uint32_t                       drawBase,
                      uint32_t                       drawCount,
                      uint64_t                       materialBufferAddr,
                      const std::string&             shaderKey   = "",
                      uint32_t                       bucketIndex = 0);
    void ExecuteLate(ICommandBuffer*                cmd,
                     const SpecializedPipelinePair& pipelinePair,
                     uint64_t                       sceneFrameAddr,
                     uint32_t                       drawBase,
                     uint32_t                       drawCount,
                     uint32_t                       pyramidSlot,
                     uint64_t                       materialBufferAddr,
                     const std::string&             shaderKey   = "",
                     uint32_t                       bucketIndex = 0);

private:
    WRef<IBuffer> m_indirectCmd   = nullptr;
    WRef<IBuffer> m_indirectCount = nullptr;
};

} // namespace Yogi
