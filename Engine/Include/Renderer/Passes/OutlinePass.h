#pragma once

#include "Renderer/Passes/OutlinePush.h"
#include "Renderer/Passes/RenderPass.h"
#include "Renderer/RHI/IPipeline.h"
#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/RHI/ITextureView.h"

namespace Yogi
{

class YG_API OutlinePass : public RenderPass
{
public:
    OutlinePass();
    ~OutlinePass() override = default;

    void Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder) override;
    void Execute(RenderGraphContext& context) override;

private:
    Owner<IShaderResourceBinding> CreateDepthBinding() const;

private:
    WRef<IPipeline>               m_pipeline       = nullptr;
    Owner<IShaderResourceBinding> m_depthBinding   = nullptr;
    const ITextureView*           m_lastBoundDepth = nullptr;
};

} // namespace Yogi
