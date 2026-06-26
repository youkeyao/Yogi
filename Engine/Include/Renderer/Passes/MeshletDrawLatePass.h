#pragma once

#include "Renderer/Passes/MeshletDrawPush.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"

namespace Yogi
{

class YG_API MeshletDrawLatePass final : public RenderPass
{
public:
    void Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder) override;
    void Execute(RenderGraphContext& context) override;

private:
    WRef<IPipeline> AcquirePipeline(const std::string& shaderKey);
    WRef<IPipeline> BuildPipeline(const std::string& shaderKey);
    void            ExecuteBucket(ICommandBuffer*   cmd,
                                  uint64_t          sceneFrameAddr,
                                  const DrawBucket& bucket,
                                  uint32_t          pyramidSlot,
                                  uint32_t          bucketIndex,
                                  uint32_t          readIdx,
                                  uint32_t          writeIdx);

private:
    WRef<IBuffer>       m_indirectCmd            = nullptr;
    WRef<IBuffer>       m_indirectCount          = nullptr;
    WRef<IBuffer>       m_visibleDrawIndexBuffer = nullptr;
    WRef<IBuffer>       m_meshletVis[2]          = { nullptr, nullptr };
    const ITextureView* m_depthPyramid           = nullptr;
    uint32_t            m_readIdx                = 0;
    uint32_t            m_writeIdx               = 1;
    WRef<IBuffer>       m_vertexBuffer           = nullptr;
    WRef<IBuffer>       m_meshletBuffer          = nullptr;
    WRef<IBuffer>       m_meshletDataBuffer      = nullptr;
    WRef<IBuffer>       m_meshDataBuffer         = nullptr;
    WRef<IBuffer>       m_meshDrawBuffer         = nullptr;

    std::unordered_map<std::string, WRef<IPipeline>> m_pipelineCache;
};

} // namespace Yogi
