#pragma once

#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/ITextureView.h"
#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/IPipeline.h"
#include "Math/Vector.h"

namespace Yogi
{

class YG_API DepthPyramid
{
public:
    DepthPyramid() = default;
    ~DepthPyramid();

    DepthPyramid(const DepthPyramid&)            = delete;
    DepthPyramid& operator=(const DepthPyramid&) = delete;
    DepthPyramid(DepthPyramid&&)                 = default;
    DepthPyramid& operator=(DepthPyramid&&)      = default;

    bool Resize(uint32_t sourceWidth, uint32_t sourceHeight);
    void Reset();

    bool IsValid() const { return m_texture != nullptr; }

    void Build(ICommandBuffer* commandBuffer, const IPipeline* reducePipeline, const ITextureView* sourceView);

    const ITexture*     GetTexture() const { return m_texture.Get(); }
    const ITextureView* GetTextureView() const { return m_textureView.Get(); }
    uint32_t            GetMipCount() const { return m_mipCount; }
    uint32_t            GetWidth() const { return m_width; }
    uint32_t            GetHeight() const { return m_height; }
    uint32_t            GetMipWidth(uint32_t mip) const { return m_mipSizes[mip].x; }
    uint32_t            GetMipHeight(uint32_t mip) const { return m_mipSizes[mip].y; }

    uint32_t GetPyramidSampledSlot() const { return m_pyramidSampledSlot; }

    static Owner<IShaderResourceBinding> CreateReduceBindingLayout();

    static std::vector<PushConstantRange> GetReducePushConstantRanges();

private:
    WRef<ITexture>                   m_texture;
    Owner<ITextureView>              m_textureView;
    std::vector<Owner<ITextureView>> m_mipViews;

    std::vector<Owner<IShaderResourceBinding>> m_mipBindings;
    std::vector<Vector<2, uint32_t>>           m_mipSizes;
    uint32_t                                   m_mipCount = 0;
    uint32_t                                   m_width    = 0;
    uint32_t                                   m_height   = 0;

    uint32_t            m_pyramidSampledSlot        = ~0u;
    const ITextureView* m_lastBoundDepth            = nullptr;
    bool                m_initialLayoutTransitioned = false;
};

} // namespace Yogi
