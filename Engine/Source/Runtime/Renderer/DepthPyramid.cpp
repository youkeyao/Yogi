#include "Renderer/DepthPyramid.h"

#include "Resources/ResourceManager/ResourceManager.h"
#include "Math/MathUtils.h"

namespace Yogi
{

namespace
{
// Cap the mip count at 16 (covers 65536x65536 sources), which is well within the limits
// of common hardware and keeps the per-frame descriptor allocations bounded.
constexpr uint32_t kMaxPyramidMips = 16;

// Push constant layout mirrors DepthReduce.comp: uvec2 imageSize (8 bytes).
struct DepthReducePushConstants
{
    uint32_t ImageWidth;
    uint32_t ImageHeight;
};

uint32_t ComputeMipCount(uint32_t w, uint32_t h)
{
    uint32_t maxDim = MathUtils::Max(w, h);
    uint32_t mips   = 1;
    while (maxDim > 1 && mips < kMaxPyramidMips)
    {
        maxDim >>= 1;
        ++mips;
    }
    return mips;
}
} // namespace

Owner<IShaderResourceBinding> DepthPyramid::CreateReduceBindingLayout()
{
    // binding 0: combined image sampler (source depth or previous pyramid mip)
    // binding 1: storage image r32f   (current pyramid mip)
    // push constants: uvec2 imageSize -> 8 bytes, compute-only
    return Owner<IShaderResourceBinding>::Create(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::Texture, ShaderStage::Compute },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::StorageImage, ShaderStage::Compute } },
        std::vector<PushConstantRange>{
            PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(DepthReducePushConstants)) } });
}

bool DepthPyramid::Resize(uint32_t sourceWidth, uint32_t sourceHeight)
{
    if (sourceWidth == 0 || sourceHeight == 0)
    {
        Reset();
        return false;
    }

    // Mip 0 dimensions = previousPow2(source). This makes every reduction step an exact 2:1
    // downsample which keeps the footprint aligned on integer texel boundaries.
    uint32_t width  = MathUtils::Max(1u, MathUtils::PreviousPow2(sourceWidth));
    uint32_t height = MathUtils::Max(1u, MathUtils::PreviousPow2(sourceHeight));

    if (m_texture && m_width == width && m_height == height)
    {
        return false;
    }

    Reset();

    m_width    = width;
    m_height   = height;
    m_mipCount = ComputeMipCount(width, height);

    // R32_FLOAT storage+sampled texture with a full mip chain. Sampler uses MAX reduction
    // mode -- this is a no-op fallback when samplerFilterMinmax is unsupported because the
    // DepthReduce.comp shader still does a manual 4-tap max (see DepthReduce.comp comments).
    TextureDesc desc{};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = m_mipCount;
    desc.Format     = ITexture::Format::R32_FLOAT;
    desc.Usage      = ITexture::Usage::Storage;
    desc.NumSamples = SampleCountFlagBits::Count1;
    desc.Reduction  = ITexture::SamplerReductionMode::Max;
    m_texture       = ResourceManager::CreateResource<ITexture>(desc);

    m_mipSizes.clear();
    m_mipSizes.reserve(m_mipCount);
    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        m_mipSizes.emplace_back(MathUtils::Max(1u, width >> mip), MathUtils::Max(1u, height >> mip));
    }

    // One descriptor set per mip. Wire binding 1 (the write target) here since it never
    // changes for the lifetime of the pyramid. Binding 0 (the sampled source) is set in
    // Build() because mip 0 reads the *scene* depth texture, not the pyramid itself.
    m_mipBindings.clear();
    m_mipBindings.reserve(m_mipCount);
    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        Owner<IShaderResourceBinding> binding = CreateReduceBindingLayout();
        binding->BindTexture(m_texture.Get(), 1, 0, mip);
        m_mipBindings.push_back(std::move(binding));
    }

    m_lastBoundDepth            = nullptr;
    m_initialLayoutTransitioned = false;
    return true;
}

void DepthPyramid::Reset()
{
    m_mipBindings.clear();
    m_mipSizes.clear();
    m_texture                   = nullptr;
    m_mipCount                  = 0;
    m_width                     = 0;
    m_height                    = 0;
    m_lastBoundDepth            = nullptr;
    m_initialLayoutTransitioned = false;
}

void DepthPyramid::Build(ICommandBuffer* commandBuffer, const IPipeline* reducePipeline, const ITexture* sourceDepth)
{
    if (!m_texture || !reducePipeline || !sourceDepth)
        return;

    ICommandBuffer* cmd = commandBuffer;

    // One-time Undefined -> General for the pyramid storage image. After this transition the
    // image stays in General forever; per-mip Barrier()s only synchronise memory, not layout.
    if (!m_initialLayoutTransitioned)
    {
        cmd->Barrier(BarrierDesc{
            .Texture      = m_texture.Get(),
            .BeforeState  = ResourceState::None,
            .AfterState   = ResourceState::UnorderedAccess,
            .BaseMipLevel = 0,
            .LevelCount   = m_mipCount,
        });
        m_initialLayoutTransitioned = true;
    }

    // Rewrite binding 0 only when the underlying source depth identity changes. For a stable
    // scene (constant target resolution, same depth texture) this runs at most once.
    if (sourceDepth != m_lastBoundDepth)
    {
        m_mipBindings[0]->BindTexture(sourceDepth, 0, 0, 0);
        for (uint32_t mip = 1; mip < m_mipCount; ++mip)
        {
            m_mipBindings[mip]->BindTexture(m_texture.Get(), 0, 0, mip - 1);
        }
        m_lastBoundDepth = sourceDepth;
    }

    cmd->SetPipeline(reducePipeline);

    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        uint32_t dstWidth  = m_mipSizes[mip].x;
        uint32_t dstHeight = m_mipSizes[mip].y;

        const IShaderResourceBinding* mipBinding = m_mipBindings[mip].Get();
        cmd->SetShaderResourceBinding(mipBinding);

        DepthReducePushConstants push{};
        push.ImageWidth  = dstWidth;
        push.ImageHeight = dstHeight;
        cmd->SetPushConstants(mipBinding, ShaderStage::Compute, 0, sizeof(DepthReducePushConstants), &push);

        constexpr uint32_t kGroup = 32;
        uint32_t           gx     = (dstWidth + kGroup - 1) / kGroup;
        uint32_t           gy     = (dstHeight + kGroup - 1) / kGroup;
        cmd->Dispatch(gx, gy, 1);

        // Between mips: make mip N's storage write visible as a sampled read for mip N+1.
        // The image stays in General throughout (see DepthPyramid.h header contract) so this
        // is a pure memory-visibility barrier -- oldLayout == newLayout == GENERAL.
        // NOTE: transitioning mip N to SHADER_READ_ONLY_OPTIMAL here would break the next frame,
        // because m_mipBindings[*]'s StorageImage descriptor (binding 1) is written with
        // imageLayout=GENERAL (see VulkanShaderResourceBinding.cpp); mismatched current layout
        // vs descriptor-recorded layout triggers VUID-vkCmdDraw-None-09600.
        if (mip + 1 < m_mipCount)
        {
            cmd->Barrier(BarrierDesc{
                .Texture      = m_texture.Get(),
                .BeforeState  = ResourceState::UnorderedAccess,
                .AfterState   = ResourceState::UnorderedAccess,
                .BaseMipLevel = mip,
                .LevelCount   = 1,
            });
        }
    }
}

} // namespace Yogi
