#pragma once

#include "Renderer/Passes/ObjectCullCommon.h"
#include "Renderer/Passes/ObjectCullPush.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"

namespace Yogi
{

class YG_API ObjectCullEarlyPass final : public RenderPass
{
public:
    ObjectCullEarlyPass();

    void Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder) override;
    void Execute(RenderGraphContext& context) override;

private:
    void ExecuteBucket(ICommandBuffer*   cmd,
                       uint64_t          sceneFrameAddr,
                       const DrawBucket& bucket,
                       uint32_t          bucketIndex,
                       uint32_t          readIdx,
                       uint32_t          writeIdx);

private:
    WRef<IPipeline> m_pipeline = nullptr;

    WRef<IBuffer> m_indirectCommandBuffer  = nullptr;
    WRef<IBuffer> m_indirectCountBuffer    = nullptr;
    WRef<IBuffer> m_visibleDrawIndexBuffer = nullptr;
    WRef<IBuffer> m_objectVis[2]           = { nullptr, nullptr };
    WRef<IBuffer> m_meshletVis[2]          = { nullptr, nullptr };
    uint32_t      m_readIdx                = 0;
    uint32_t      m_writeIdx               = 1;
    WRef<IBuffer> m_meshDataBuffer         = nullptr;
    WRef<IBuffer> m_meshDrawBuffer         = nullptr;
    WRef<IBuffer> m_drawIndexBuffer        = nullptr;
};

} // namespace Yogi
