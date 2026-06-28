#include "Renderer/Passes/HiZPass.h"
#include "Renderer/BindlessTextureManager.h"
#include "Renderer/RenderGraph.h"
#include "Renderer/ShaderData.h"
#include "Math/MathUtils.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

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

HiZPass::HiZPass()
{
    WRef<IShaderResourceBinding> depthReduceBindingTemplate = ResourceManager::AddResource(CreateReduceBinding());
    WRef<ShaderDesc>             depthReduceShader =
        AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Passes/DepthReduce.cs.slang");

    PipelineDesc depthReducePipelineDesc{};
    depthReducePipelineDesc.Type               = PipelineType::Compute;
    depthReducePipelineDesc.Shaders            = { depthReduceShader.Get() };
    depthReducePipelineDesc.ResourceBinding    = depthReduceBindingTemplate.Get();
    depthReducePipelineDesc.PushConstantRanges = { PushConstantRange{
        ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(DepthReducePushConstants)) } };
    m_depthReducePipeline = ResourceManager::AcquireSharedResource<IPipeline>(depthReducePipelineDesc);
}

HiZPass::~HiZPass()
{
    m_depthReducePipeline = nullptr;
    Reset();
    m_output = nullptr;
}

void HiZPass::Init(RenderGraph& graph)
{
    m_output = &graph.RegisterTexture("HiZ.DepthPyramid");
}

void HiZPass::RebuildIfNeeded(const RenderGraphContext& context)
{
    if (!m_output || !context.DepthView)
        return;

    const uint32_t sourceWidth  = context.DepthView->GetTexture()->GetWidth();
    const uint32_t sourceHeight = context.DepthView->GetTexture()->GetHeight();
    if (sourceWidth == 0 || sourceHeight == 0)
    {
        Reset();
        return;
    }

    const uint32_t width  = MathUtils::Max(1u, MathUtils::PreviousPow2(sourceWidth));
    const uint32_t height = MathUtils::Max(1u, MathUtils::PreviousPow2(sourceHeight));

    const ITexture* currentTexture = *m_output ? (*m_output)->GetTexture() : nullptr;
    if (currentTexture && m_width == width && m_height == height)
        return;

    Reset();

    m_width    = width;
    m_height   = height;
    m_mipCount = ComputeMipCount(width, height);

    TextureDesc desc{};
    desc.Width      = width;
    desc.Height     = height;
    desc.MipLevels  = m_mipCount;
    desc.Format     = Format::R32_FLOAT;
    desc.NumSamples = SampleCountFlagBits::Count1;
    desc.UsageFlags = TextureUsageFlags::Storage | TextureUsageFlags::Sampled;

    WRef<ITexture> texture = ResourceManager::CreateResource<ITexture>(desc);
    m_mipSizes.clear();
    m_mipSizes.reserve(m_mipCount);
    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
        m_mipSizes.emplace_back(MathUtils::Max(1u, width >> mip), MathUtils::Max(1u, height >> mip));

    m_mipViews.clear();
    m_mipViews.reserve(m_mipCount);
    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
        m_mipViews.push_back(ITextureView::Create(texture,
                                                  TextureViewDesc{ .BaseMipLevel    = mip,
                                                                   .MipLevelCount   = 1,
                                                                   .BaseArrayLayer  = 0,
                                                                   .ArrayLayerCount = 1,
                                                                   .Format          = Format::NONE }));
    *m_output = ITextureView::Create(texture);

    m_mipBindings.clear();
    m_mipBindings.reserve(m_mipCount);
    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        Owner<IShaderResourceBinding> binding = CreateReduceBinding();
        binding->BindTextureView(m_mipViews[mip].Get(), 2, 0);
        m_mipBindings.push_back(std::move(binding));
    }

    m_pyramidSampledSlot = BindlessTextureManager::Register(m_output->Get());
    m_lastBoundDepth     = nullptr;
}

void HiZPass::Reset()
{
    if (BindlessTextureManager::IsInitialized() && m_pyramidSampledSlot != BindlessTextureManager::INVALID_SLOT)
        BindlessTextureManager::Unregister(m_pyramidSampledSlot);
    m_pyramidSampledSlot = BindlessTextureManager::INVALID_SLOT;

    m_mipBindings.clear();
    m_mipViews.clear();
    m_mipSizes.clear();
    m_mipCount       = 0;
    m_width          = 0;
    m_height         = 0;
    m_lastBoundDepth = nullptr;

    if (m_output)
    {
        *m_output = nullptr;
    }
}

void HiZPass::Build(ICommandBuffer* commandBuffer, const ITextureView* sourceView)
{
    if (!m_output || !*m_output || !m_depthReducePipeline || !sourceView)
        return;

    ICommandBuffer* cmd = commandBuffer;
    cmd->BeginDebugLabel("HiZ::Build");

    if (sourceView != m_lastBoundDepth)
    {
        m_mipBindings[0]->BindTextureView(sourceView, 1, 0);
        for (uint32_t mip = 1; mip < m_mipCount; ++mip)
            m_mipBindings[mip]->BindTextureView(m_mipViews[mip - 1].Get(), 1, 0);
        m_lastBoundDepth = sourceView;
    }

    cmd->SetPipeline(m_depthReducePipeline.Get());

    for (uint32_t mip = 0; mip < m_mipCount; ++mip)
    {
        uint32_t dstWidth  = m_mipSizes[mip].x;
        uint32_t dstHeight = m_mipSizes[mip].y;

        cmd->SetShaderResourceBinding(m_mipBindings[mip].Get());

        DepthReducePushConstants push{};
        push.ImageWidth  = dstWidth;
        push.ImageHeight = dstHeight;
        cmd->SetPushConstants(
            m_depthReducePipeline.Get(), ShaderStage::Compute, 0, sizeof(DepthReducePushConstants), &push);

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

    cmd->EndDebugLabel();
}

void HiZPass::Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder)
{
    RebuildIfNeeded(context);

    if (context.DepthView && *m_output && m_depthReducePipeline)
    {
        builder.UseTexture(m_output->Get(), ResourceState::UnorderedAccess);
        builder.UseTexture(context.DepthView, ResourceState::ComputeShaderResource);
    }
}

void HiZPass::Execute(RenderGraphContext& context)
{
    if (m_output && *m_output)
        Build(context.CommandBuffer, context.DepthView);
}

Owner<IShaderResourceBinding> HiZPass::CreateReduceBinding() const
{
    return Owner<IShaderResourceBinding>::Create(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::Sampler, ShaderStage::Compute },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::SampledTexture, ShaderStage::Compute },
            ShaderResourceAttribute{ 2, 1, ShaderResourceType::StorageTexture, ShaderStage::Compute } },
        std::vector<ImmutableSamplerBindingDesc>{ ImmutableSamplerBindingDesc{
            0, 1, ShaderStage::Compute, SamplerDesc{ .Reduction = SamplerReductionMode::Max } } });
}

} // namespace Yogi
