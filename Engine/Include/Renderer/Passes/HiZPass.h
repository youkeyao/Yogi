#pragma once

#include "Renderer/Passes/RenderPass.h"
#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/ITextureView.h"
#include "Math/Vector.h"

namespace Yogi
{

class YG_API HiZPass : public RenderPass
{
public:
    HiZPass();
    ~HiZPass() override;

    void Init(RenderGraph& graph) override;
    void Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder) override;
    void Execute(RenderGraphContext& context) override;

private:
    Owner<IShaderResourceBinding> CreateReduceBinding() const;
    void                          RebuildIfNeeded(const RenderGraphContext& context);
    void                          Reset();
    void                          Build(ICommandBuffer* commandBuffer, const ITextureView* sourceView);

private:
    WRef<IPipeline>      m_depthReducePipeline = nullptr;
    Owner<ITextureView>* m_output              = nullptr;

    std::vector<Owner<ITextureView>>           m_mipViews;
    std::vector<Owner<IShaderResourceBinding>> m_mipBindings;
    std::vector<Vector<2, uint32_t>>           m_mipSizes;

    uint32_t            m_mipCount           = 0;
    uint32_t            m_width              = 0;
    uint32_t            m_height             = 0;
    uint32_t            m_pyramidSampledSlot = ~0u;
    const ITextureView* m_lastBoundDepth     = nullptr;
};

} // namespace Yogi
