#include "Renderer/Passes/OutlinePass.h"
#include "Renderer/RenderGraph.h"
#include "Core/Application.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

OutlinePass::OutlinePass()
{
    WRef<IShaderResourceBinding> bindingTemplate = ResourceManager::AddResource(CreateDepthBinding());

    WRef<ShaderDesc> vs = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Passes/Outline.vs.slang");
    WRef<ShaderDesc> fs = AssetManager::AcquireAsset<ShaderDesc>("EngineAssets/Shaders/Passes/Outline.fs.slang");
    if (!vs.Get() || !fs.Get())
    {
        YG_CORE_ERROR("OutlinePass: shader load failed");
        return;
    }

    PipelineDesc desc{};
    desc.Type               = PipelineType::Graphics;
    desc.Shaders            = { vs.Get(), fs.Get() };
    desc.ResourceBinding    = bindingTemplate.Get();
    desc.PushConstantRanges = { PushConstantRange{
        ShaderStage::Fragment, 0, static_cast<uint32_t>(sizeof(OutlinePush)) } };
    desc.DepthFormat        = Format::NONE;
    desc.ColorTargets       = { ColorTargetDesc{
        .Format      = Application::GetInstance().GetSwapChain()->GetColorFormat(),
        .ColorBlend  = { BlendFactor::SrcAlpha, BlendFactor::OneMinusSrcAlpha, BlendOp::Add },
        .AlphaBlend  = { BlendFactor::One, BlendFactor::Zero, BlendOp::Add },
        .EnableBlend = true,
        .WriteMask   = ColorWriteMask::All,
    } };
    desc.Samples            = SampleCountFlagBits::Count1;
    desc.Topology           = PrimitiveTopology::TriangleList;
    desc.Cull               = CullMode::None;
    desc.DepthTestEnable    = false;
    desc.DepthWriteEnable   = false;

    m_pipeline       = ResourceManager::AcquireSharedResource<IPipeline>(desc);
    m_depthBinding   = CreateDepthBinding();
    m_lastBoundDepth = nullptr;
}

Owner<IShaderResourceBinding> OutlinePass::CreateDepthBinding() const
{
    return Owner<IShaderResourceBinding>::Create(
        std::vector<ShaderResourceAttribute>{
            ShaderResourceAttribute{ 0, 1, ShaderResourceType::Sampler, ShaderStage::Fragment },
            ShaderResourceAttribute{ 1, 1, ShaderResourceType::SampledTexture, ShaderStage::Fragment } },
        std::vector<ImmutableSamplerBindingDesc>{
            ImmutableSamplerBindingDesc{ 0, 1, ShaderStage::Fragment, SamplerDesc{} } });
}

void OutlinePass::Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder)
{
    (void)graph;
    if (!m_pipeline || !context.CurrentTarget || !context.DepthView)
        return;

    builder.UseTexture(context.CurrentTarget, ResourceState::ColorAttachment);
    builder.UseTexture(context.DepthView, ResourceState::FragmentShaderResource);
}

void OutlinePass::Execute(RenderGraphContext& context)
{
    if (!m_pipeline || !context.CurrentTarget || !context.DepthView || !m_depthBinding)
        return;

    if (context.DepthView != m_lastBoundDepth)
    {
        m_depthBinding->BindTextureView(context.DepthView, 1, 0);
        m_lastBoundDepth = context.DepthView;
    }

    RenderingDesc rdesc{};
    rdesc.Width   = context.TargetWidth;
    rdesc.Height  = context.TargetHeight;
    rdesc.Samples = SampleCountFlagBits::Count1;

    RenderingAttachment color{};
    color.View        = context.CurrentTarget;
    color.LoadAction  = LoadOp::Load;
    color.StoreAction = StoreOp::Store;
    rdesc.ColorAttachments.push_back(color);
    // No depth attachment: this is a fullscreen overlay.

    ICommandBuffer* cmd = context.CommandBuffer;
    cmd->BeginDebugLabel("OutlinePass::Execute");
    cmd->BeginRendering(rdesc);
    cmd->SetViewport({ 0, 0, static_cast<float>(context.TargetWidth), static_cast<float>(context.TargetHeight) });
    cmd->SetScissor({ 0, 0, context.TargetWidth, context.TargetHeight });

    cmd->SetPipeline(m_pipeline.Get());
    cmd->SetShaderResourceBinding(m_depthBinding.Get());

    OutlinePush pc{};
    pc.SceneFrameAddr = context.SceneFrameAddr;
    pc.Thickness      = 1.5f;
    pc.DepthThreshold = 0.02f;
    pc.Color[0]       = 0.0f;
    pc.Color[1]       = 0.0f;
    pc.Color[2]       = 0.0f;
    cmd->SetPushConstants(m_pipeline.Get(), ShaderStage::Fragment, 0, sizeof(OutlinePush), &pc);

    cmd->Draw(3, 1, 0, 0);

    cmd->EndRendering();
    cmd->EndDebugLabel();
}

} // namespace Yogi
