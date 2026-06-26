#include "Renderer/Passes/ObjectCullEarlyPass.h"
#include "Renderer/RenderGraph.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

ObjectCullEarlyPass::ObjectCullEarlyPass()
{
    WRef<ShaderDesc> shader = AssetManager::AcquireAsset<ShaderDesc>(ObjectCull::SHADER_EARLY);
    PipelineDesc     desc{};
    desc.Type               = PipelineType::Compute;
    desc.PushConstantRanges = { PushConstantRange{
        ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(ObjectCullPush)) } };
    desc.ResourceBinding    = nullptr;
    desc.Shaders            = { shader.Get() };
    m_pipeline              = ResourceManager::AcquireSharedResource<IPipeline>(desc);
}

void ObjectCullEarlyPass::Prepare(RenderGraphContext& context, RenderGraph& graph, RenderGraphBuilder& builder)
{
    (void)context;
    m_indirectCommandBuffer  = graph.GetBuffer("ObjectCull.IndirectCommand");
    m_indirectCountBuffer    = graph.GetBuffer("ObjectCull.IndirectCount");
    m_visibleDrawIndexBuffer = graph.GetBuffer("ObjectCull.VisibleDrawIndex");
    m_objectVis[0]           = graph.GetBuffer("ObjectCull.ObjectVis0");
    m_objectVis[1]           = graph.GetBuffer("ObjectCull.ObjectVis1");
    m_meshletVis[0]          = graph.GetBuffer("ObjectCull.MeshletVis0");
    m_meshletVis[1]          = graph.GetBuffer("ObjectCull.MeshletVis1");
    m_meshDataBuffer         = graph.GetBuffer("Geometry.MeshData");
    m_meshDrawBuffer         = graph.GetBuffer("Geometry.MeshDraw");
    m_drawIndexBuffer        = graph.GetBuffer("Geometry.DrawIndex");

    m_readIdx  = 1u - m_readIdx;
    m_writeIdx = 1u - m_readIdx;

    builder.UseBuffer(m_indirectCountBuffer.Get(), ResourceState::UnorderedAccess);
    builder.UseBuffer(m_objectVis[m_writeIdx].Get(), ResourceState::UnorderedAccess);
    builder.UseBuffer(m_meshletVis[m_writeIdx].Get(), ResourceState::UnorderedAccess);
    builder.UseBuffer(m_indirectCommandBuffer.Get(), ResourceState::UnorderedAccess);
    builder.UseBuffer(m_visibleDrawIndexBuffer.Get(), ResourceState::UnorderedAccess);
    builder.UseBuffer(m_objectVis[m_readIdx].Get(), ResourceState::ComputeShaderResource);
    builder.UseBuffer(m_meshDataBuffer.Get(), ResourceState::ComputeShaderResource);
    builder.UseBuffer(m_meshDrawBuffer.Get(), ResourceState::ComputeShaderResource);
    builder.UseBuffer(m_drawIndexBuffer.Get(), ResourceState::ComputeShaderResource);
}

void ObjectCullEarlyPass::Execute(RenderGraphContext& context)
{
    if (!context.Buckets)
        return;

    for (size_t bi = 0; bi < context.Buckets->size(); ++bi)
    {
        const DrawBucket& b = (*context.Buckets)[bi];
        ExecuteBucket(
            context.CommandBuffer, context.SceneFrameAddr, b, static_cast<uint32_t>(bi), m_readIdx, m_writeIdx);
    }
}

void ObjectCullEarlyPass::ExecuteBucket(ICommandBuffer*   cmd,
                                        uint64_t          sceneFrameAddr,
                                        const DrawBucket& bucket,
                                        uint32_t          bucketIndex,
                                        uint32_t          readIdx,
                                        uint32_t          writeIdx)
{
    if (bucket.DrawCount == 0)
        return;

    cmd->BeginDebugLabel("ObjectCullEarly::Execute");
    cmd->SetPipeline(m_pipeline.Get());

    ObjectCullPush pcCull{};
    pcCull.SceneFrameAddr         = sceneFrameAddr;
    pcCull.VisibleDrawIndexBuffer = m_visibleDrawIndexBuffer->GetDeviceAddress();
    pcCull.IndirectCommandBuffer  = m_indirectCommandBuffer->GetDeviceAddress();
    pcCull.IndirectCountBuffer    = m_indirectCountBuffer->GetDeviceAddress();
    pcCull.ObjectVisBufferRead    = m_objectVis[readIdx]->GetDeviceAddress();
    pcCull.ObjectVisBufferWrite   = m_objectVis[writeIdx]->GetDeviceAddress();
    pcCull.MeshDataBuffer         = m_meshDataBuffer->GetDeviceAddress();
    pcCull.MeshDrawBuffer         = m_meshDrawBuffer->GetDeviceAddress();
    pcCull.DrawIndexBuffer        = m_drawIndexBuffer->GetDeviceAddress();
    pcCull.DrawBase               = bucket.DrawBase;
    pcCull.DrawCount              = bucket.DrawCount;
    pcCull.BucketIndex            = bucketIndex;
    cmd->SetPushConstants(m_pipeline.Get(), ShaderStage::Compute, 0, sizeof(ObjectCullPush), &pcCull);

    const uint32_t dispatchX = (bucket.DrawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
    cmd->Dispatch(dispatchX, 1, 1);

    cmd->EndDebugLabel();
}

} // namespace Yogi
