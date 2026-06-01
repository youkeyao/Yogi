#pragma once

#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ICommandBuffer.h"

namespace Yogi
{

class YG_API StagingArena
{
public:
    StagingArena() = default;

    // blockSize = bytes per host-visible block. The arena allocates additional
    // blocks of this size on demand when no idle block has room for a request
    // (or larger, when a single request exceeds blockSize).
    void Init(uint64_t blockSize);
    void Shutdown();

    // Optional: eager poll + reset of any block whose owning cmd buffer has
    // finished. Called once per frame keeps the steady-state pool minimal;
    // skipping it just means FindOrAlloc will lazy-poll on the next Stage.
    void BeginFrame();

    // Copy `size` bytes of `data` into the staging arena and append a
    // vkCmdCopyBuffer to `cmd` that uploads them to `dst` at `dstOffset`. The
    // block holding the staged bytes is tagged with `cmd` so it stays reserved
    // until cmd's fence signals completion.
    //
    // Returns the device address of the staged bytes (rarely useful -- most
    // callers only care that the GPU-side dst now contains the data).
    uint64_t Stage(ICommandBuffer* cmd, IBuffer* dst, uint64_t dstOffset,
                   const void* data, uint64_t size, uint64_t alignment = 16);

    // Niagara-style direct upload: copy `size` bytes into the staging arena
    // and return the BDA pointing AT THE STAGING BYTES. No CopyBuffer is
    // recorded -- the GPU reads from host-visible memory through the
    // returned address (via buffer_reference / BDA in the shader).
    //
    // Use for small per-frame structs (SceneFrame, push-constant payloads
    // referenced by BDA) where the device-local-copy round-trip costs more
    // than the host-visible read amortized over a few thousand cache-friendly
    // dereferences. Not for hot bulk data (vertex / meshlet) -- that should
    // still go through Stage(...) into device-local memory.
    //
    // The block is tagged with `cmd` because the shader read happens during
    // cmd's GPU execution -- block becomes recyclable when cmd's fence signals.
    uint64_t Push(ICommandBuffer* cmd, const void* data, uint64_t size, uint64_t alignment = 16);

    // Reserve `size` bytes in the staging arena up-front and return a CPU
    // pointer for the caller to write into. Pair with CommitMappedStage to
    // actually record the CopyBuffer. The block is tagged with `cmd` at
    // BeginMappedStage time -- if Commit is never called the block stays
    // reserved until cmd is done, which is harmless.
    void* BeginMappedStage(ICommandBuffer* cmd, uint64_t size, uint64_t alignment = 16);
    void  CommitMappedStage(ICommandBuffer* cmd, IBuffer* dst, uint64_t dstOffset, uint64_t size);

private:
    static uint64_t AlignUp(uint64_t v, uint64_t a) { return (v + a - 1) & ~(a - 1); }

    struct Block
    {
        WRef<IBuffer>   Buffer;
        void*           Mapped       = nullptr;
        uint64_t        Capacity     = 0;
        uint64_t        Head         = 0;
        // Last command buffer that recorded a CopyBuffer reading from this
        // block. nullptr means the block is fresh / has been recycled. While
        // non-null, the block is unusable for any other cmd buffer until
        // LastUserCmd->IsFinished() returns true.
        ICommandBuffer* LastUserCmd  = nullptr;
    };

    // Find an existing block usable for (cmd, size, alignment), or allocate
    // a new one. "Usable" means the block is either fresh, owned by a cmd
    // that's already finished (lazily reset here), or owned by `cmd` itself
    // and has room.
    Block& FindOrAllocBlockForSize(ICommandBuffer* cmd, uint64_t size, uint64_t alignment);
    Block& AllocateNewBlock(uint64_t minSize);

    std::vector<Block> m_blocks;
    uint64_t           m_blockSize = 0;

    // Pending mapped-stage state (BeginMappedStage / CommitMappedStage pair).
    Block*   m_pendingBlock       = nullptr;
    uint64_t m_pendingBlockOffset = 0;
};

} // namespace Yogi
