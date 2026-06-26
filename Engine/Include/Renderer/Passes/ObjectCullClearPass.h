#pragma once

#include "Renderer/Passes/ObjectCullCommon.h"
#include "Renderer/Passes/RenderPass.h"

namespace Yogi
{

class YG_API ObjectCullClearPass final : public RenderPass
{
public:
    void Init(RenderGraph& graph) override;
    void Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder) override;
    void Execute(RenderGraphContext& context) override;

private:
    WRef<IBuffer> m_indirectCountBuffer = nullptr;
    WRef<IBuffer> m_objectVis[2]        = { nullptr, nullptr };
    WRef<IBuffer> m_meshletVis[2]       = { nullptr, nullptr };
    uint32_t      m_readIdx             = 0;
    uint32_t      m_writeIdx            = 1;
};

} // namespace Yogi
