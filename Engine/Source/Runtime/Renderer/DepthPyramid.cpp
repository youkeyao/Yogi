#include "Renderer/DepthPyramid.h"

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

uint32_t PreviousPow2(uint32_t v)
{
    uint32_t r = 1;
    while ((r << 1) < v)
        r <<= 1;
    return r;
}

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
    // binding 0: combined image sampler (source depth or previous pyramid mip)
    // binding 1: storage image r32f   (current pyramid mip)
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

    // Pyramid mip 0 = previousPow2(source). Forces every later reduction to be a
    // clean 2:1 divide -- otherwise Vulkan's truncating mip-extent rule would drop
    // the last odd row/column of an intermediate mip from the max reduction.
    uint32_t width  = MathUtils::Max(1u, PreviousPow2(sourceWidth));
    uint32_t height = MathUtils::Max(1u, PreviousPow2(sourceHeight));

    if (m_texture && m_width == width && m_height == height)
        return false;

    Reset();

    m_width    = width;
    m_height   = height;
    m_mipCount = ComputeMipCount(width, height);

    // R32_FLOAT storage+sampled with a full mip chain. SamplerReductionMode::Max
    // makes textureLod() do a 2x2 max in hardware, which DepthReduce.comp relies
    // on for parent->child reduction.
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

    // One descriptor set per mip. Binding 1 (write target) is fixed for the lifetime
    // of the pyramid; binding 0 (sampled source) is wired in Build() because mip 0
    // reads the scene depth texture, not the pyramid itself.
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

void DepthPyramid::EnsureInitialLayout(ICommandBuffer* commandBuffer)
{
    if (!m_texture || m_initialLayoutTransitioned)
        return;

    commandBuffer->Barrier(BarrierDesc{
        .Texture      = m_texture.Get(),
        .BeforeState  = ResourceState::None,
        .AfterState   = ResourceState::UnorderedAccess,
        .BaseMipLevel = 0,
        .LevelCount   = m_mipCount,
    });
    m_initialLayoutTransitioned = true;
}

void DepthPyramid::Build(ICommandBuffer* commandBuffer, const IPipeline* reducePipeline, const ITexture* sourceDepth)
{
    if (!m_texture || !reducePipeline || !sourceDepth)
        return;

    ICommandBuffer* cmd = commandBuffer;

    // One-time Undefined -> General; the pyramid stays in General for its lifetime.
    EnsureInitialLayout(cmd);

    // Rebind only when the underlying depth texture identity changes.
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

        // Make mip N's writes visible as a sampled read for mip N+1. Layout stays
        // GENERAL (matches the StorageImage descriptor's recorded layout); switching
        // to SHADER_READ_ONLY_OPTIMAL here would trip VUID-vkCmdDraw-None-09600 next
        // frame.
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
