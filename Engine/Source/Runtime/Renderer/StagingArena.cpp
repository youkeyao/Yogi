#include "Renderer/StagingArena.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

void StagingArena::Init(uint64_t blockSize)
{
    m_blockSize = AlignUp(blockSize, 256ull);
    m_blocks.clear();
    // Lazy alloc: first Stage call allocates the first block. Eager alloc
    // would need to know up-front the typical per-frame upload size; lazy
    // gets the size right by definition.
}

void StagingArena::Shutdown()
{
    m_blocks.clear();
    m_blockSize          = 0;
    m_pendingBlock       = nullptr;
    m_pendingBlockOffset = 0;
}

void StagingArena::BeginFrame()
{
    m_pendingBlock       = nullptr;
    m_pendingBlockOffset = 0;
    // Eager poll. Not strictly required for correctness (FindOrAlloc lazy-
    // polls too), but lets the steady-state pool stay minimal: without it,
    // a stale tag on a free block would force a new allocation if the
    // *first* Stage of the frame walks past it.
    for (Block& b : m_blocks)
    {
        if (b.LastUserCmd && b.LastUserCmd->IsFinished())
        {
            b.Head        = 0;
            b.LastUserCmd = nullptr;
        }
    }
}

StagingArena::Block& StagingArena::AllocateNewBlock(uint64_t minSize)
{
    Block b{};
    b.Capacity = AlignUp(minSize, 256ull);
    b.Buffer   = ResourceManager::CreateResource<IBuffer>(BufferDesc{ b.Capacity, BufferUsage::Staging });
    b.Mapped   = b.Buffer->GetMappedPtr();
    YG_CORE_ASSERT(b.Mapped, "StagingArena: staging block was not host-visible");
    b.Head        = 0;
    b.LastUserCmd = nullptr;
    m_blocks.push_back(std::move(b));
    return m_blocks.back();
}

StagingArena::Block& StagingArena::FindOrAllocBlockForSize(ICommandBuffer* cmd, uint64_t size, uint64_t alignment)
{
    for (Block& b : m_blocks)
    {
        // A block is reusable for `cmd` if either (a) it's already being
        // appended to by `cmd` itself, or (b) its previous owner has finished.
        ICommandBuffer* owner = b.LastUserCmd;
        if (owner && owner != cmd)
        {
            if (!owner->IsFinished())
                continue; // still in flight on a different cmd, can't touch
            // Lazy reclaim path -- BeginFrame may have been skipped.
            b.Head        = 0;
            b.LastUserCmd = nullptr;
        }
        const uint64_t off = AlignUp(b.Head, alignment);
        if (off + size <= b.Capacity)
            return b;
    }
    // Need to grow. Size new block to fit at least our request.
    const uint64_t need = size + alignment;
    return AllocateNewBlock(need > m_blockSize ? need : m_blockSize);
}

uint64_t StagingArena::Stage(ICommandBuffer* cmd, IBuffer* dst, uint64_t dstOffset,
                             const void* data, uint64_t size, uint64_t alignment)
{
    YG_CORE_ASSERT(cmd && dst && data, "StagingArena: Stage called with null arg");
    YG_CORE_ASSERT(size > 0, "StagingArena: Stage called with zero size");

    Block&         b   = FindOrAllocBlockForSize(cmd, size, alignment);
    const uint64_t off = AlignUp(b.Head, alignment);
    YG_CORE_ASSERT(off + size <= b.Capacity, "StagingArena: block overflow after FindOrAllocBlockForSize");

    memcpy(static_cast<char*>(b.Mapped) + off, data, size);
    b.Head        = off + size;
    b.LastUserCmd = cmd;

    cmd->CopyBuffer(b.Buffer.Get(), dst, off, dstOffset, size);

    return b.Buffer->GetDeviceAddress() + off;
}

uint64_t StagingArena::Push(ICommandBuffer* cmd, const void* data, uint64_t size, uint64_t alignment)
{
    YG_CORE_ASSERT(cmd && data, "StagingArena: Push called with null arg");
    YG_CORE_ASSERT(size > 0, "StagingArena: Push called with zero size");

    Block&         b   = FindOrAllocBlockForSize(cmd, size, alignment);
    const uint64_t off = AlignUp(b.Head, alignment);
    YG_CORE_ASSERT(off + size <= b.Capacity, "StagingArena: block overflow after FindOrAllocBlockForSize");

    memcpy(static_cast<char*>(b.Mapped) + off, data, size);
    b.Head        = off + size;
    b.LastUserCmd = cmd;
    // No CopyBuffer recorded -- caller will read directly via the returned BDA.
    return b.Buffer->GetDeviceAddress() + off;
}

void* StagingArena::BeginMappedStage(ICommandBuffer* cmd, uint64_t size, uint64_t alignment)
{
    YG_CORE_ASSERT(cmd, "StagingArena: BeginMappedStage called with null cmd");
    YG_CORE_ASSERT(size > 0, "StagingArena: BeginMappedStage called with zero size");
    YG_CORE_ASSERT(!m_pendingBlock, "StagingArena: nested BeginMappedStage without Commit");

    Block&         b   = FindOrAllocBlockForSize(cmd, size, alignment);
    const uint64_t off = AlignUp(b.Head, alignment);

    m_pendingBlock       = &b;
    m_pendingBlockOffset = off;
    b.Head               = off + size;
    b.LastUserCmd        = cmd;

    return static_cast<char*>(b.Mapped) + off;
}

void StagingArena::CommitMappedStage(ICommandBuffer* cmd, IBuffer* dst, uint64_t dstOffset, uint64_t size)
{
    YG_CORE_ASSERT(m_pendingBlock, "StagingArena: CommitMappedStage without BeginMappedStage");
    YG_CORE_ASSERT(cmd && dst, "StagingArena: CommitMappedStage with null arg");
    YG_CORE_ASSERT(m_pendingBlock->LastUserCmd == cmd,
                   "StagingArena: CommitMappedStage cmd differs from BeginMappedStage cmd");

    cmd->CopyBuffer(m_pendingBlock->Buffer.Get(), dst, m_pendingBlockOffset, dstOffset, size);
    m_pendingBlock       = nullptr;
    m_pendingBlockOffset = 0;
}

} // namespace Yogi
