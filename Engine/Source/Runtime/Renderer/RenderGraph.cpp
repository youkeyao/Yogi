#include "Renderer/RenderGraph.h"
#include "Renderer/Passes/RenderPass.h"

namespace Yogi
{

void RenderGraphBuilder::UseBuffer(const IBuffer* buffer, ResourceState state)
{
    if (!buffer)
        return;
    m_bufferUses.push_back(BufferUse{ buffer, state });
}

void RenderGraphBuilder::UseTexture(const ITextureView* view, ResourceState state)
{
    if (!view)
        return;
    m_textureUses.push_back(TextureUse{ view, state });
}

void RenderGraphContext::SetInitialTextureState(const ITextureView* view, ResourceState state)
{
    if (!view)
        return;
    TextureInitialStates.push_back(TextureInitialState{ view, state });
}

void RenderGraph::Clear()
{
    m_ownedPasses.clear();
    m_passes.clear();
    m_frameBuilders.clear();
    m_buffers.clear();
    m_textures.clear();
    ResetStateTracking();
    m_textureBeginStates.clear();
}

void RenderGraph::Execute(ICommandBuffer* commandBuffer, RenderGraphContext context)
{
    YG_CORE_ASSERT(commandBuffer, "RenderGraph::Execute: null command buffer");
    ResetStateTracking();
    SeedTextureStates(context);

    context.CommandBuffer = commandBuffer;

    m_frameBuilders.clear();
    m_frameBuilders.resize(m_passes.size());
    for (size_t i = 0; i < m_passes.size(); ++i)
    {
        YG_CORE_ASSERT(m_passes[i].Pass, "RenderGraph::Execute: null pass");
        m_passes[i].Pass->Prepare(context, *this, m_frameBuilders[i]);
    }

    for (size_t i = 0; i < m_passes.size(); ++i)
    {
        PassEntry&          entry   = m_passes[i];
        RenderGraphBuilder& builder = m_frameBuilders[i];

        for (const RenderGraphBuilder::BufferUse& use : builder.m_bufferUses)
            RequireBufferState(commandBuffer, use.Buffer, use.State);

        for (const RenderGraphBuilder::TextureUse& use : builder.m_textureUses)
            RequireTextureState(commandBuffer, use.View, use.State);

        commandBuffer->BeginDebugLabel(entry.Label.c_str());
        entry.Pass->Execute(context);

        for (const RenderGraphBuilder::BufferUse& use : builder.m_bufferUses)
            m_bufferStates[use.Buffer] = use.State;

        commandBuffer->EndDebugLabel();
    }

    if (context.TransitionToPresent)
        RequireTextureState(commandBuffer, context.CurrentTarget, ResourceState::Present);

    m_textureBeginStates = m_textureStates;
}

void RenderGraph::RegisterBuffer(const std::string& name, WRef<IBuffer> buffer)
{
    YG_CORE_ASSERT(buffer, "RenderGraph::RegisterBuffer: null buffer");
    m_buffers[name] = buffer;
}

WRef<IBuffer> RenderGraph::GetBuffer(const std::string& name) const
{
    auto it = m_buffers.find(name);
    if (it == m_buffers.end())
        return nullptr;
    return it->second;
}

Owner<ITextureView>& RenderGraph::RegisterTexture(const std::string& name)
{
    return m_textures[name];
}

const ITextureView* RenderGraph::GetTexture(const std::string& name) const
{
    auto it = m_textures.find(name);
    if (it == m_textures.end())
        return nullptr;
    return it->second.Get();
}

void RenderGraph::ResetStateTracking()
{
    m_bufferStates.clear();
    m_textureStates.clear();
}

void RenderGraph::SeedTextureStates(const RenderGraphContext& context)
{
    m_textureStates = m_textureBeginStates;
    for (const RenderGraphContext::TextureInitialState& initialState : context.TextureInitialStates)
    {
        if (initialState.View)
            m_textureStates[initialState.View] = initialState.State;
    }
}

void RenderGraph::RequireBufferState(ICommandBuffer* commandBuffer,
                                     const IBuffer*  buffer,
                                     ResourceState   after,
                                     ResourceState   initial)
{
    if (!buffer || after == ResourceState::None)
        return;

    auto          it     = m_bufferStates.find(buffer);
    ResourceState before = it == m_bufferStates.end() ? initial : it->second;
    if (before == after)
        return;

    commandBuffer->Barrier(BarrierDesc{
        .Buffer      = buffer,
        .BeforeState = before,
        .AfterState  = after,
    });
    m_bufferStates[buffer] = after;
}

void RenderGraph::RequireTextureState(ICommandBuffer* commandBuffer, const ITextureView* view, ResourceState after)
{
    if (!view || after == ResourceState::None)
        return;

    auto          it     = m_textureStates.find(view);
    ResourceState before = it == m_textureStates.end() ? ResourceState::Undefined : it->second;
    if (before == after)
        return;

    commandBuffer->Barrier(BarrierDesc{
        .TextureView = view,
        .BeforeState = before,
        .AfterState  = after,
    });
    m_textureStates[view] = after;
}

} // namespace Yogi
