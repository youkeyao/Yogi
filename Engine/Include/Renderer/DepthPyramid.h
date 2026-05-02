#pragma once

#include "Renderer/RHI/ITexture.h"
#include "Renderer/RHI/IShaderResourceBinding.h"
#include "Renderer/RHI/ICommandBuffer.h"
#include "Renderer/RHI/IPipeline.h"
#include "Math/Vector.h"

namespace Yogi
{

class YG_API DepthPyramid
{
public:
    DepthPyramid()  = default;
    ~DepthPyramid() = default;

    DepthPyramid(const DepthPyramid&)            = delete;
    DepthPyramid& operator=(const DepthPyramid&) = delete;
    DepthPyramid(DepthPyramid&&)                 = default;
    DepthPyramid& operator=(DepthPyramid&&)      = default;

    bool Resize(uint32_t sourceWidth, uint32_t sourceHeight);
    void Reset();

    bool IsValid() const { return m_texture != nullptr; }

    void Build(ICommandBuffer* commandBuffer, const IPipeline* reducePipeline, const ITexture* sourceDepth);

    const ITexture* GetTexture() const { return m_texture.Get(); }
    uint32_t        GetMipCount() const { return m_mipCount; }
    uint32_t        GetWidth() const { return m_width; }
    uint32_t        GetHeight() const { return m_height; }
    uint32_t        GetMipWidth(uint32_t mip) const { return m_mipSizes[mip].x; }
    uint32_t        GetMipHeight(uint32_t mip) const { return m_mipSizes[mip].y; }

    static Owner<IShaderResourceBinding> CreateReduceBindingLayout();

private:
    WRef<ITexture>                             m_texture;
    std::vector<Owner<IShaderResourceBinding>> m_mipBindings;
    std::vector<Vector<2, uint32_t>>           m_mipSizes;
    uint32_t                                   m_mipCount = 0;
    uint32_t                                   m_width    = 0;
    uint32_t                                   m_height   = 0;

    const ITexture* m_lastBoundDepth            = nullptr;
    bool            m_initialLayoutTransitioned = false;
};

} // namespace Yogi
