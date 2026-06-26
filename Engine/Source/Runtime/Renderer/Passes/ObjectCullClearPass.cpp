#include "Renderer/Passes/ObjectCullClearPass.h"
#include "Renderer/RenderGraph.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

void ObjectCullClearPass::Init(RenderGraph& graph)
{
    graph.RegisterBuffer("ObjectCull.IndirectCommand",
                         ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{
                             ObjectCull::INDIRECT_COMMAND_BUFFER_SIZE, BufferUsage::Storage | BufferUsage::Indirect }));
    graph.RegisterBuffer("ObjectCull.IndirectCount",
                         ResourceManager::AcquireSharedResource<IBuffer>(BufferDesc{
                             ObjectCull::INDIRECT_COUNT_BUFFER_SIZE, BufferUsage::Storage | BufferUsage::Indirect }));
    graph.RegisterBuffer("ObjectCull.VisibleDrawIndex",
                         ResourceManager::AcquireSharedResource<IBuffer>(
                             BufferDesc{ ObjectCull::VISIBLE_DRAW_INDEX_BUFFER_SIZE, BufferUsage::Storage }));
    graph.RegisterBuffer("ObjectCull.ObjectVis0",
                         ResourceManager::AcquireSharedResource<IBuffer>(
                             BufferDesc{ ObjectCull::OBJECT_VIS_BITFIELD_SIZE, BufferUsage::Storage }));
    graph.RegisterBuffer("ObjectCull.ObjectVis1",
                         ResourceManager::AcquireSharedResource<IBuffer>(
                             BufferDesc{ ObjectCull::OBJECT_VIS_BITFIELD_SIZE, BufferUsage::Storage }));
    graph.RegisterBuffer("ObjectCull.MeshletVis0",
                         ResourceManager::AcquireSharedResource<IBuffer>(
                             BufferDesc{ ObjectCull::MESHLET_VIS_BITFIELD_SIZE, BufferUsage::Storage }));
    graph.RegisterBuffer("ObjectCull.MeshletVis1",
                         ResourceManager::AcquireSharedResource<IBuffer>(
                             BufferDesc{ ObjectCull::MESHLET_VIS_BITFIELD_SIZE, BufferUsage::Storage }));
}

void ObjectCullClearPass::Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder)
{
    m_indirectCountBuffer = graph.GetBuffer("ObjectCull.IndirectCount");
    m_objectVis[0]        = graph.GetBuffer("ObjectCull.ObjectVis0");
    m_objectVis[1]        = graph.GetBuffer("ObjectCull.ObjectVis1");
    m_meshletVis[0]       = graph.GetBuffer("ObjectCull.MeshletVis0");
    m_meshletVis[1]       = graph.GetBuffer("ObjectCull.MeshletVis1");

    Owner<ICommandBuffer> initCmd =
        Owner<ICommandBuffer>::Create(CommandBufferDesc{ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Graphics });
    initCmd->Begin();
    for (int i = 0; i < 2; ++i)
    {
        initCmd->FillBuffer(m_objectVis[i].Get(), 0, ObjectCull::OBJECT_VIS_BITFIELD_SIZE, 0u);
        initCmd->FillBuffer(m_meshletVis[i].Get(), 0, ObjectCull::MESHLET_VIS_BITFIELD_SIZE, 0u);
    }
    initCmd->FillBuffer(m_indirectCountBuffer.Get(), 0, ObjectCull::INDIRECT_COUNT_BUFFER_SIZE, 0u);
    initCmd->End();
    initCmd->Submit();

    m_readIdx  = 1u - m_readIdx;
    m_writeIdx = 1u - m_readIdx;

    builder.UseBuffer(m_indirectCountBuffer.Get(), ResourceState::CopyDestination);
    builder.UseBuffer(m_objectVis[m_writeIdx].Get(), ResourceState::CopyDestination);
    builder.UseBuffer(m_meshletVis[m_writeIdx].Get(), ResourceState::CopyDestination);
}

void ObjectCullClearPass::Execute(RenderGraphContext& context)
{
    ICommandBuffer* cmd = context.CommandBuffer;
    YG_CORE_ASSERT(cmd, "ObjectCullClearPass::Execute: null command buffer");

    cmd->FillBuffer(m_indirectCountBuffer.Get(), 0, ObjectCull::INDIRECT_COUNT_BUFFER_SIZE, 0u);
    cmd->FillBuffer(m_objectVis[m_writeIdx].Get(), 0, ObjectCull::OBJECT_VIS_BITFIELD_SIZE, 0u);
    cmd->FillBuffer(m_meshletVis[m_writeIdx].Get(), 0, ObjectCull::MESHLET_VIS_BITFIELD_SIZE, 0u);
}

} // namespace Yogi
