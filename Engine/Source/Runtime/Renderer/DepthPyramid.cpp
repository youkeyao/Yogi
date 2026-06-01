#include "Renderer/DepthPyramid.h"
#include "Renderer/BindlessTextures.h"
#include "Renderer/ShaderData.h"

#include "Resources/ResourceManager/ResourceManager.h"
#include "Math/MathUtils.h"

namespace Yogi
{

namespace
{
struct DepthReducePushConstants
{
    uint32_t ImageWidth;
    uint32_t ImageHeight;
};

uint32_t ComputeMipCount(uint32_t w, uint32_t h)
{
    uint32_t maxDim = MathUtils::Max(w, h);
    uint32_t mips   = 1;
    while (maxDim > 1 && mips < 16)
    {
        maxDim >>= 1;
        ++mips;
    }
    return mips;
}
} // namespace

Owner<IShaderResourceBinding> DepthPyramid::CreateReduceBindingLayout()
{
    return Owner<IShaderResourceBinding>::Create(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::Sampler, ShaderStage::Compute },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::SampledTexture, ShaderStage::Compute },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageTexture, ShaderStage::Compute } },
        std::vector<ImmutableSamplerDesc>{ ImmutableSamplerDesc{ 0, SamplerReductionMode::Max } });
}

std::vector<PushConstantRange> DepthPyramid::GetReducePushConstantRanges()
{
    return { PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(DepthReducePushConstants)) } };
}

DepthPyramid::~DepthPyramid()
{
    if (BindlessTextures::IsInitialized() && m_pyramidSampledSlot != BindlessTextures::INVALID_SLOT)
    {
        BindlessTextures::Get().Unregister(m_pyramidSampledSlot);
    }
}

bool DepthPyramid::Resize(uint32_t sourceWidth, uint32_t sourceHeight)
{
    if (sourceWidth == 0 || sourceHeight == 0)
    {
        Reset();
        return false;
    }

    // Pyramid mip 0 = previousPow2(source). Forces every later reduction to be a clean 2:1 divide
    uint32_t width  = MathUtils::Max(1u, MathUtils::PreviousPow2(sourceWidth));
    uint32_t height = MathUtils::Max(1u, MathUtils::PreviousPow2(sourceHeight));

    if (m_texture && m_width == width && m_height == height)
        return false;

    Reset();

    m_width    = width;
    m_height   = height;
    m_mipCount = ComputeMipCount(width, height);

    TextureDesc desc{};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = m_mipCount;
    desc.Format     = ITexture::Format::R32_FLOAT;
    desc.Usage      = ITexture::Usage::Storage;
    desc.NumSamples = SampleCountFlagBits::Count1;
    m_texture       = ResourceManager::CreateResource<ITexture>(desc);

    m_mipSizes.clear();
    m_mipSizes.reserve(m_mipCount);
    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        m_mipSizes.emplace_back(MathUtils::Max(1u, width >> mip), MathUtils::Max(1u, height >> mip));
    }

    // One single-mip view per mip level; one full-pyramid view spanning all mips.
    m_mipViews.clear();
    m_mipViews.reserve(m_mipCount);
    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        m_mipViews.push_back(
            ITextureView::Create(m_texture, TextureViewDesc{ /*BaseMipLevel*/ mip, /*MipLevelCount*/ 1 }));
    }
    m_textureView = ITextureView::Create(m_texture);

    // One SRB per mip, with binding=2 (storage write target) wired up here
    // (it never changes). binding=1 (sampled src image) gets wired in Build();
    // binding=0 is the immutable max-reduction sampler -- baked into the
    // layout, no descriptor write needed.
    m_mipBindings.clear();
    m_mipBindings.reserve(m_mipCount);
    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        Owner<IShaderResourceBinding> binding = CreateReduceBindingLayout();
        binding->BindTextureView(m_mipViews[mip].Get(), 2, 0);
        m_mipBindings.push_back(std::move(binding));
    }

    // Pyramid sampled view goes into the global bindless pool so LATE passes
    // can reach it via texture slot indices (same as material textures).
    if (BindlessTextures::IsInitialized())
        m_pyramidSampledSlot = BindlessTextures::Get().Register(m_textureView.Get());

    m_lastBoundDepth            = nullptr;
    m_initialLayoutTransitioned = false;
    return true;
}

void DepthPyramid::Reset()
{
    if (BindlessTextures::IsInitialized() && m_pyramidSampledSlot != BindlessTextures::INVALID_SLOT)
    {
        BindlessTextures::Get().Unregister(m_pyramidSampledSlot);
    }
    m_pyramidSampledSlot = BindlessTextures::INVALID_SLOT;

    m_mipBindings.clear();
    m_mipViews.clear();
    m_textureView = nullptr;
    m_mipSizes.clear();
    m_texture                   = nullptr;
    m_mipCount                  = 0;
    m_width                     = 0;
    m_height                    = 0;
    m_lastBoundDepth            = nullptr;
    m_initialLayoutTransitioned = false;
}

void DepthPyramid::EnsureInitialLayout(ICommandBuffer* commandBuffer)
{
    if (!m_texture || m_initialLayoutTransitioned)
        return;

    commandBuffer->Barrier(BarrierDesc{
        .TextureView = m_textureView.Get(),
        .BeforeState = ResourceState::None,
        .AfterState  = ResourceState::UnorderedAccess,
    });
    m_initialLayoutTransitioned = true;
}

void DepthPyramid::Build(ICommandBuffer* commandBuffer, const IPipeline* reducePipeline, const ITextureView* sourceView)
{
    if (!m_texture || !reducePipeline || !sourceView)
        return;

    ICommandBuffer* cmd = commandBuffer;

    // One-time Undefined -> General; the pyramid stays in General for its lifetime.
    EnsureInitialLayout(cmd);

    // Rewire mip 0's sampled src only when the underlying depth view changes
    // (window resize / first frame). mip 1+ are wired once at Resize-time
    // because they read the pyramid's own previous mip, which doesn't move.
    if (sourceView != m_lastBoundDepth)
    {
        m_mipBindings[0]->BindTextureView(sourceView, 1, 0);
        for (uint32_t mip = 1; mip < m_mipCount; ++mip)
        {
            m_mipBindings[mip]->BindTextureView(m_mipViews[mip - 1].Get(), 1, 0);
        }
        m_lastBoundDepth = sourceView;
    }

    cmd->SetPipeline(reducePipeline);

    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        uint32_t dstWidth  = m_mipSizes[mip].x;
        uint32_t dstHeight = m_mipSizes[mip].y;

        cmd->SetShaderResourceBinding(m_mipBindings[mip].Get());

        DepthReducePushConstants push{};
        push.ImageWidth  = dstWidth;
        push.ImageHeight = dstHeight;
        cmd->SetPushConstants(reducePipeline, ShaderStage::Compute, 0, sizeof(DepthReducePushConstants), &push);

        uint32_t gx = (dstWidth + DEPTH_REDUCE_WGSIZE - 1) / DEPTH_REDUCE_WGSIZE;
        uint32_t gy = (dstHeight + DEPTH_REDUCE_WGSIZE - 1) / DEPTH_REDUCE_WGSIZE;
        cmd->Dispatch(gx, gy, 1);

        if (mip + 1 < m_mipCount)
        {
            cmd->Barrier(BarrierDesc{
                .TextureView = m_mipViews[mip].Get(),
                .BeforeState = ResourceState::UnorderedAccess,
                .AfterState  = ResourceState::UnorderedAccess | ResourceState::ComputeShaderResource,
            });
        }
    }
}

} // namespace Yogi
