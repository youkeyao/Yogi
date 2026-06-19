#include "Renderer/Passes/ObjectCullPass.h"
#include "Renderer/BindlessTextureManager.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

const char* ObjectCullPass::k_objectCullShaderEarly = "EngineAssets/Shaders/Passes/ObjectCull.cs.slang";
const char* ObjectCullPass::k_objectCullShaderLate  = "EngineAssets/Shaders/Passes/ObjectCull.cs.slang::LATE=1";

SpecializedPipelineBuilder ObjectCullPass::GetPipelineBuilder()
{
    return [](const PassKey& /*pass*/, const std::string& /*materialTypeName*/) -> SpecializedPipelinePair {
        YG_CORE_WARN("ObjectCullPass::GetPipelineBuilder called -- this pass is not "
                     "expected to be registered with PipelineRegistry.");
        return {};
    };
}

void ObjectCullPass::Initialize()
{
    m_indirectCommandBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ INDIRECT_COMMAND_BUFFER_SIZE, BufferUsage::Storage | BufferUsage::Indirect });
    m_indirectCountBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ INDIRECT_COUNT_BUFFER_SIZE, BufferUsage::Storage | BufferUsage::Indirect });
    m_visibleDrawIndexBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ VISIBLE_DRAW_INDEX_BUFFER_SIZE, BufferUsage::Storage });

    for (int i = 0; i < 2; ++i)
    {
        m_objectVis[i] = ResourceManager::AcquireSharedResource<IBuffer>(
            BufferDesc{ OBJECT_VIS_BITFIELD_SIZE, BufferUsage::Storage });
        m_meshletVis[i] = ResourceManager::AcquireSharedResource<IBuffer>(
            BufferDesc{ MESHLET_VIS_BITFIELD_SIZE, BufferUsage::Storage });
    }
    m_visReadIdx = 0;

    WRef<ShaderDesc> cullShaderEarly = AssetManager::AcquireAsset<ShaderDesc>(k_objectCullShaderEarly);
    WRef<ShaderDesc> cullShaderLate  = AssetManager::AcquireAsset<ShaderDesc>(k_objectCullShaderLate);

    PipelineDesc desc{};
    desc.Type               = PipelineType::Compute;
    desc.PushConstantRanges = { PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(CullPush)) } };

    desc.ResourceBinding = nullptr; // EARLY: no textures
    desc.Shaders         = { cullShaderEarly.Get() };
    m_cullPipelineEarly  = ResourceManager::AcquireSharedResource<IPipeline>(desc);

    desc.ResourceBinding = BindlessTextureManager::GetSRB(); // LATE: Hi-Z sample
    desc.Shaders         = { cullShaderLate.Get() };
    m_cullPipelineLate   = ResourceManager::AcquireSharedResource<IPipeline>(desc);

    {
        Owner<ICommandBuffer> initCmd = Owner<ICommandBuffer>::Create(
            CommandBufferDesc{ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Graphics });
        initCmd->Begin();
        for (int i = 0; i < 2; ++i)
        {
            initCmd->FillBuffer(m_objectVis[i].Get(), 0, OBJECT_VIS_BITFIELD_SIZE, 0u);
            initCmd->FillBuffer(m_meshletVis[i].Get(), 0, MESHLET_VIS_BITFIELD_SIZE, 0u);
        }
        initCmd->FillBuffer(m_indirectCountBuffer.Get(), 0, INDIRECT_COUNT_BUFFER_SIZE, 0u);
        initCmd->End();
        initCmd->Submit();
    }
}

void ObjectCullPass::Shutdown()
{
    m_cullPipelineEarly      = nullptr;
    m_cullPipelineLate       = nullptr;
    m_indirectCommandBuffer  = nullptr;
    m_indirectCountBuffer    = nullptr;
    m_visibleDrawIndexBuffer = nullptr;
    for (int i = 0; i < 2; ++i)
    {
        m_objectVis[i]  = nullptr;
        m_meshletVis[i] = nullptr;
    }
}

void ObjectCullPass::BeginFrame()
{
    m_visReadIdx = 1 - m_visReadIdx;
}

void ObjectCullPass::FillSceneFrame(SceneFrame& sf)
{
    sf.IndirectCommandBuffer  = m_indirectCommandBuffer->GetDeviceAddress();
    sf.IndirectCountBuffer    = m_indirectCountBuffer->GetDeviceAddress();
    sf.VisibleDrawIndexBuffer = m_visibleDrawIndexBuffer->GetDeviceAddress();

    const uint32_t readIdx   = m_visReadIdx;
    const uint32_t writeIdx  = 1u - m_visReadIdx;
    sf.ObjectVisBufferRead   = m_objectVis[readIdx]->GetDeviceAddress();
    sf.ObjectVisBufferWrite  = m_objectVis[writeIdx]->GetDeviceAddress();
    sf.MeshletVisBufferRead  = m_meshletVis[readIdx]->GetDeviceAddress();
    sf.MeshletVisBufferWrite = m_meshletVis[writeIdx]->GetDeviceAddress();
}

void ObjectCullPass::PrepareEarly(ICommandBuffer* cmd)
{
    YG_CORE_ASSERT(cmd, "ObjectCullPass::PrepareEarly: null cmd");
    cmd->BeginDebugLabel("ObjectCull::PrepareEarly");

    const uint32_t writeIdx = 1u - m_visReadIdx;

    cmd->FillBuffer(m_indirectCountBuffer.Get(), 0, INDIRECT_COUNT_BUFFER_SIZE, 0u);
    cmd->FillBuffer(m_objectVis[writeIdx].Get(), 0, OBJECT_VIS_BITFIELD_SIZE, 0u);
    cmd->FillBuffer(m_meshletVis[writeIdx].Get(), 0, MESHLET_VIS_BITFIELD_SIZE, 0u);

    cmd->Barrier(ResourceState::CopyDestination | ResourceState::UnorderedAccess,
                 ResourceState::UnorderedAccess | ResourceState::ComputeShaderResource |
                     ResourceState::TaskShaderResource);

    cmd->EndDebugLabel();
}

void ObjectCullPass::ExecuteEarly(ICommandBuffer* cmd,
                                  uint64_t        sceneFrameAddr,
                                  uint32_t        drawBase,
                                  uint32_t        drawCount,
                                  uint32_t        bucketIndex)
{
    if (drawCount == 0)
        return;
    cmd->BeginDebugLabel("ObjectCull::ExecuteEarly");

    cmd->SetPipeline(m_cullPipelineEarly.Get());

    CullPush pcCull{};
    pcCull.SceneFrameAddr = sceneFrameAddr;
    pcCull.DrawBase       = drawBase;
    pcCull.DrawCount      = drawCount;
    pcCull.BucketIndex    = bucketIndex;
    cmd->SetPushConstants(m_cullPipelineEarly.Get(), ShaderStage::Compute, 0, sizeof(CullPush), &pcCull);

    const uint32_t dispatchX = (drawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
    cmd->Dispatch(dispatchX, 1, 1);

    cmd->Barrier(ResourceState::UnorderedAccess,
                 ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource);

    cmd->EndDebugLabel();
}

void ObjectCullPass::PrepareLate(ICommandBuffer* cmd)
{
    YG_CORE_ASSERT(cmd, "ObjectCullPass::PrepareLate: null cmd");
    cmd->BeginDebugLabel("ObjectCull::PrepareLate");

    // NOTE: m_indirectCountBuffer is NOT cleared here — EARLY and LATE
    // both increment the same per-bucket counters, and the total visible
    // count is EARLY_count + LATE_count (mutually exclusive draw sets).
    cmd->Barrier(ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource,
                 ResourceState::UnorderedAccess);

    cmd->EndDebugLabel();
}

void ObjectCullPass::ExecuteLate(ICommandBuffer* cmd,
                                 uint64_t        sceneFrameAddr,
                                 uint32_t        drawBase,
                                 uint32_t        drawCount,
                                 uint32_t        pyramidSlot,
                                 uint32_t        bucketIndex)
{
    if (drawCount == 0)
        return;
    cmd->BeginDebugLabel("ObjectCull::ExecuteLate");

    cmd->SetPipeline(m_cullPipelineLate.Get());
    cmd->SetShaderResourceBinding(BindlessTextureManager::GetSRB());

    CullPush pcCull{};
    pcCull.SceneFrameAddr = sceneFrameAddr;
    pcCull.DrawBase       = drawBase;
    pcCull.DrawCount      = drawCount;
    pcCull.PyramidSlot    = pyramidSlot;
    pcCull.BucketIndex    = bucketIndex;
    cmd->SetPushConstants(m_cullPipelineLate.Get(), ShaderStage::Compute, 0, sizeof(CullPush), &pcCull);

    const uint32_t dispatchX = (drawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
    cmd->Dispatch(dispatchX, 1, 1);

    cmd->Barrier(ResourceState::UnorderedAccess,
                 ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource);

    cmd->EndDebugLabel();
}

} // namespace Yogi
