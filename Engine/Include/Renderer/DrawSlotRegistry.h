#pragma once

#include "Renderer/ShaderData.h"

namespace Yogi
{

class YG_API DrawSlotRegistry
{
public:
    enum class AcquireResult
    {
        Ok,           // outBufferIndex + outVisOffset valid; build + CommitDraw
        CapExceeded,  // MeshDraw buffer full -- caller should stop the frame
        VisExhausted, // meshlet vis bitfield full -- caller should skip this draw
    };

    struct DirtyEntry
    {
        uint32_t BufferIndex;
        MeshDraw Draw;
    };

    void Init(uint32_t maxDraws, uint32_t maxMeshletVisBits);
    void Reset();

    void BeginFrame();

    AcquireResult Acquire(uint32_t  entityHandle,
                          uint32_t  meshIndex,
                          uint32_t  meshletCount,
                          uint32_t& outBufferIndex,
                          uint32_t& outVisOffset);

    void CommitDraw(uint32_t entityHandle, const MeshDraw& draw, std::vector<DirtyEntry>& dirtyDraws);

    void ReclaimUnseen(std::vector<DirtyEntry>& dirtyDraws);

    uint32_t GetDrawCount() const { return m_drawSlotHighWater; }

private:
    struct DrawSlot
    {
        uint32_t BufferIndex      = 0;
        uint32_t MeshletVisOffset = 0;
        uint32_t MeshletVisCount  = 0;           // for Free; unrounded mesh meshlet count
        uint32_t LastMeshIndex    = 0xFFFFFFFFu; // tracks mesh-on-slot changes
        MeshDraw LastUploaded     = {};
        bool     LastUploadValid  = false;
        bool     SeenThisFrame    = false;
    };

    class VisRangeAllocator
    {
    public:
        static constexpr uint32_t INVALID = 0xFFFFFFFFu;

        void     Init(uint32_t capacity);
        void     Reset();
        uint32_t Alloc(uint32_t count);
        void     Free(uint32_t offset, uint32_t count);

    private:
        struct FreeRange
        {
            uint32_t Offset;
            uint32_t Count;
        };
        std::vector<FreeRange> m_free; // sorted by Offset
        uint32_t               m_capacity = 0;
        uint32_t               m_used     = 0; // bump pointer
    };

    std::unordered_map<uint32_t, DrawSlot> m_drawSlots;             // key = entity handle (uint32_t)
    std::vector<uint32_t>                  m_freeBufferIndices;     // BufferIndex free list (LIFO)
    uint32_t                               m_drawSlotHighWater = 0; // max BufferIndex ever assigned + 1
    uint32_t                               m_maxDraws          = 0; // BufferIndex capacity
    VisRangeAllocator                      m_visRangeAllocator;
};

} // namespace Yogi
