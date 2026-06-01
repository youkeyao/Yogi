#include "Renderer/ObjectCullPass.h"
#include "Renderer/BindlessTextures.h"
#include "Resources/AssetManager/AssetManager.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

const char* ObjectCullPass::k_objectCullShaderEarly = "EngineAssets/Shaders/ObjectCull.comp";
const char* ObjectCullPass::k_objectCullShaderLate  = "EngineAssets/Shaders/ObjectCull.comp::LATE=1";

void ObjectCullPass::Initialize()
{
    // ---- Indirect-flow buffers (single-region; cleared between phases) ----
    m_indirectCommandBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ INDIRECT_COMMAND_BUFFER_SIZE, BufferUsage::Storage | BufferUsage::Indirect });
    m_indirectCountBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ INDIRECT_COUNT_BUFFER_SIZE, BufferUsage::Storage | BufferUsage::Indirect });
    m_visibleDrawIndexBuffer = ResourceManager::AcquireSharedResource<IBuffer>(
        BufferDesc{ VISIBLE_DRAW_INDEX_BUFFER_SIZE, BufferUsage::Storage });

    // ---- Double-buffered, bit-packed visibility bitfields ----
    for (int i = 0; i < 2; ++i)
    {
        m_objectVis[i]  = ResourceManager::AcquireSharedResource<IBuffer>(
            BufferDesc{ OBJECT_VIS_BITFIELD_SIZE, BufferUsage::Storage });
        m_meshletVis[i] = ResourceManager::AcquireSharedResource<IBuffer>(
            BufferDesc{ MESHLET_VIS_BITFIELD_SIZE, BufferUsage::Storage });
    }
    m_visReadIdx = 0;

    // ---- Pipelines ----
    IShaderResourceBinding* bindlessSRB = BindlessTextures::IsInitialized() ? BindlessTextures::Get().GetSRB() : nullptr;
    YG_CORE_ASSERT(bindlessSRB, "ObjectCullPass: BindlessTextures must be initialized before Initialize()");

    WRef<ShaderDesc> cullShaderEarly = AssetManager::AcquireAsset<ShaderDesc>(k_objectCullShaderEarly);
    WRef<ShaderDesc> cullShaderLate  = AssetManager::AcquireAsset<ShaderDesc>(k_objectCullShaderLate);

    PipelineDesc desc{};
    desc.Type               = PipelineType::Compute;
    desc.ResourceBinding    = bindlessSRB;
    desc.PushConstantRanges = { PushConstantRange{ ShaderStage::Compute, 0, static_cast<uint32_t>(sizeof(CullPush)) } };

    desc.Shaders        = { cullShaderEarly.Get() };
    m_cullPipelineEarly = ResourceManager::AcquireSharedResource<IPipeline>(desc);

    desc.Shaders       = { cullShaderLate.Get() };
    m_cullPipelineLate = ResourceManager::AcquireSharedResource<IPipeline>(desc);

    // ---- One-shot zero-init of vis buffers ----
    // Device-local memory is not guaranteed zero by Vulkan; first frame's
    // EARLY phase reads the Read-side bitfield (which has never been
    // touched) and would otherwise see undefined bits. FillBuffer the four
    // vis buffers + the count slot directly on the device -- no staging
    // round-trip, no host upload.
    //
    // Submit on the Graphics queue (not Transfer) so the buffer's queue
    // family ownership matches what runtime Prepare* uses (graphics swap-
    // chain cmd buffer). With sharingMode = EXCLUSIVE, mixing transfer
    // queue init + graphics runtime access on different queue families
    // would require explicit queue family ownership transfers; staying on
    // a single family avoids that whole class of bug.
    {
        Owner<ICommandBuffer> initCmd = Owner<ICommandBuffer>::Create(
            CommandBufferDesc{ CommandBufferUsage::OneTimeSubmit, SubmitQueue::Graphics });
        initCmd->Begin();
        for (int i = 0; i < 2; ++i)
        {
            initCmd->FillBuffer(m_objectVis[i].Get(), 0, OBJECT_VIS_BITFIELD_SIZE, 0u);
            initCmd->FillBuffer(m_meshletVis[i].Get(), 0, MESHLET_VIS_BITFIELD_SIZE, 0u);
        }
        // Also zero the count slot once -- harmless if it's reset every
        // frame, but avoids an undefined first-frame indirect dispatch
        // value if anything reads it before the first PrepareEarly.
        initCmd->FillBuffer(m_indirectCountBuffer.Get(), 0, sizeof(uint32_t), 0u);
        initCmd->End();
        initCmd->Submit();
        initCmd->Wait();
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
    // Flip the double-buffered indices. Frame N's Write becomes frame N+1's
    // Read; frame N's Read (which still holds frame N-1's verdict) becomes
    // frame N+1's Write and will be cleared to zero in PrepareEarly before
    // any LATE atomicOr touches it.
    m_visReadIdx = 1 - m_visReadIdx;
}

void ObjectCullPass::FillSceneFrame(SceneFrame& sf)
{
    sf.IndirectCommandBuffer  = m_indirectCommandBuffer->GetDeviceAddress();
    sf.IndirectCountBuffer    = m_indirectCountBuffer->GetDeviceAddress();
    sf.VisibleDrawIndexBuffer = m_visibleDrawIndexBuffer->GetDeviceAddress();

    const uint32_t readIdx  = m_visReadIdx;
    const uint32_t writeIdx = 1u - m_visReadIdx;
    sf.ObjectVisBufferRead   = m_objectVis[readIdx]->GetDeviceAddress();
    sf.ObjectVisBufferWrite  = m_objectVis[writeIdx]->GetDeviceAddress();
    sf.MeshletVisBufferRead  = m_meshletVis[readIdx]->GetDeviceAddress();
    sf.MeshletVisBufferWrite = m_meshletVis[writeIdx]->GetDeviceAddress();
}

void ObjectCullPass::PrepareEarly(ICommandBuffer* cmd)
{
    YG_CORE_ASSERT(cmd, "ObjectCullPass::PrepareEarly: null cmd");

    const uint32_t writeIdx = 1u - m_visReadIdx;

    // Three FillBuffer ops into different dst buffers (count slot, write
    // ObjectVis, write MeshletVis). Pure GPU-side fills -- no staging
    // round-trip, no host upload.
    cmd->FillBuffer(m_indirectCountBuffer.Get(), 0, sizeof(uint32_t), 0u);
    cmd->FillBuffer(m_objectVis[writeIdx].Get(), 0, OBJECT_VIS_BITFIELD_SIZE, 0u);
    cmd->FillBuffer(m_meshletVis[writeIdx].Get(), 0, MESHLET_VIS_BITFIELD_SIZE, 0u);

    // Single global memory barrier instead of three per-buffer barriers.
    // Driver maps both forms to the same hardware op (cache flush + drain),
    // and validation can no longer flag a bogus mismatch between BeforeState
    // and the buffer's actual previous use because there's no per-buffer
    // claim being made.
    //
    // ObjectVisBufferRead (= the OTHER vis buffer; not zeroed here) is also
    // covered: its previous LATE-cull writes from the previous frame's
    // submission are drained by the cross-submit semaphore chain, and this
    // global barrier additionally makes any in-flight write available to
    // EARLY's compute read on this submission.
    cmd->Barrier(
        ResourceState::CopyDestination | ResourceState::UnorderedAccess,
        ResourceState::UnorderedAccess | ResourceState::ComputeShaderResource | ResourceState::TaskShaderResource);
}

void ObjectCullPass::ExecuteEarly(ICommandBuffer* cmd,
                                  uint64_t        sceneFrameAddr,
                                  uint32_t        drawBase,
                                  uint32_t        drawCount)
{
    if (drawCount == 0)
        return;

    cmd->SetPipeline(m_cullPipelineEarly.Get());
    cmd->SetShaderResourceBinding(BindlessTextures::Get().GetSRB());

    CullPush pcCull{};
    pcCull.SceneFrameAddr = sceneFrameAddr;
    pcCull.DrawBase       = drawBase;
    pcCull.DrawCount      = drawCount;
    pcCull.PyramidSlot    = 0; // unused by EARLY (no Hi-Z probing)
    cmd->SetPushConstants(m_cullPipelineEarly.Get(), ShaderStage::Compute, 0, sizeof(CullPush), &pcCull);

    const uint32_t dispatchX = (drawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
    cmd->Dispatch(dispatchX, 1, 1);

    // Compute writes (indirect cmd / count / vdi) -> indirect arg + task/mesh
    // reads. Single global barrier covers all three buffers; their distinct
    // dst stages are OR'd together so the driver gates the next stage on the
    // compute writes regardless of which exact resource it consumes first.
    cmd->Barrier(
        ResourceState::UnorderedAccess,
        ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource);
}

void ObjectCullPass::PrepareLate(ICommandBuffer* cmd)
{
    YG_CORE_ASSERT(cmd, "ObjectCullPass::PrepareLate: null cmd");

    // Count slot back to 0 for the LATE region. Indirect cmd / vdi don't
    // need clearing -- LATE simply overwrites slots [0, lateCount).
    //
    // The single global memory barrier before the copy covers both the
    // EARLY task/mesh draw's indirect read of the count buffer and the
    // copy's TRANSFER_WRITE that's about to overwrite it, plus drains the
    // EARLY task/mesh shader read of indirect cmd / vdi so LATE cull can
    // UAV-write them.
    cmd->Barrier(
        ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource,
        ResourceState::CopyDestination | ResourceState::UnorderedAccess);

    cmd->FillBuffer(m_indirectCountBuffer.Get(), 0, sizeof(uint32_t), 0u);

    // Drain the transfer write into LATE cull's UAV access.
    cmd->Barrier(ResourceState::CopyDestination, ResourceState::UnorderedAccess);
}

void ObjectCullPass::ExecuteLate(ICommandBuffer* cmd,
                                 uint64_t        sceneFrameAddr,
                                 uint32_t        drawBase,
                                 uint32_t        drawCount,
                                 uint32_t        pyramidSlot)
{
    if (drawCount == 0)
        return;

    cmd->SetPipeline(m_cullPipelineLate.Get());
    cmd->SetShaderResourceBinding(BindlessTextures::Get().GetSRB());

    CullPush pcCull{};
    pcCull.SceneFrameAddr = sceneFrameAddr;
    pcCull.DrawBase       = drawBase;
    pcCull.DrawCount      = drawCount;
    pcCull.PyramidSlot    = pyramidSlot;
    cmd->SetPushConstants(m_cullPipelineLate.Get(), ShaderStage::Compute, 0, sizeof(CullPush), &pcCull);

    const uint32_t dispatchX = (drawCount + CULL_WORKGROUP_SIZE - 1) / CULL_WORKGROUP_SIZE;
    cmd->Dispatch(dispatchX, 1, 1);

    // LATE cull writes -> indirect arg + task/mesh reads for the LATE draw.
    cmd->Barrier(
        ResourceState::UnorderedAccess,
        ResourceState::IndirectArg | ResourceState::TaskShaderResource | ResourceState::MeshShaderResource);
}

} // namespace Yogi
