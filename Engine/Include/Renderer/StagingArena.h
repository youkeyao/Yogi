#pragma once

#include "Renderer/RHI/IBuffer.h"
#include "Renderer/RHI/ICommandBuffer.h"

namespace Yogi
{

class YG_API StagingArena
{
public:
    StagingArena() = default;

    void Init(uint64_t blockSize);
    void Shutdown();

    void BeginFrame();

    uint64_t Stage(ICommandBuffer* cmd,
                   IBuffer*        dst,
                   uint64_t        dstOffset,
                   const void*     data,
                   uint64_t        size,
                   uint64_t        alignment = 16);

    uint64_t Push(ICommandBuffer* cmd, const void* data, uint64_t size, uint64_t alignment = 16);

    void* BeginMappedStage(ICommandBuffer* cmd, uint64_t size, uint64_t alignment = 16);
    void  CommitMappedStage(ICommandBuffer* cmd, IBuffer* dst, uint64_t dstOffset, uint64_t size);

private:
    static uint64_t AlignUp(uint64_t v, uint64_t a) { return (v + a - 1) & ~(a - 1); }

    struct Block
    {
        WRef<IBuffer>   Buffer;
        void*           Mapped      = nullptr;
        uint64_t        Capacity    = 0;
        uint64_t        Head        = 0;
        ICommandBuffer* LastUserCmd = nullptr;
    };

    Block& FindOrAllocBlockForSize(ICommandBuffer* cmd, uint64_t size, uint64_t alignment);
    Block& AllocateNewBlock(uint64_t minSize);

    std::vector<Block> m_blocks;
    uint64_t           m_blockSize = 0;

    Block*   m_pendingBlock       = nullptr;
    uint64_t m_pendingBlockOffset = 0;
};

} // namespace Yogi
