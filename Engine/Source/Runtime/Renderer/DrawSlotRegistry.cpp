#include "Renderer/DrawSlotRegistry.h"

namespace Yogi
{

// ---------------------------- VisRangeAllocator ----------------------------

void DrawSlotRegistry::VisRangeAllocator::Init(uint32_t capacity)
{
    m_capacity = capacity;
    m_used     = 0;
    m_free.clear();
}

void DrawSlotRegistry::VisRangeAllocator::Reset()
{
    m_used = 0;
    m_free.clear();
}

uint32_t DrawSlotRegistry::VisRangeAllocator::Alloc(uint32_t count)
{
    if (count == 0)
        return 0;

    count = (count + 31u) & ~31u;

    // First-fit on the free list.
    for (auto it = m_free.begin(); it != m_free.end(); ++it)
    {
        if (it->Count >= count)
        {
            const uint32_t offset = it->Offset;
            if (it->Count == count)
                m_free.erase(it);
            else
            {
                it->Offset += count;
                it->Count -= count;
            }
            return offset;
        }
    }

    if (m_used + count > m_capacity)
        return INVALID;
    const uint32_t offset = m_used;
    m_used += count;
    return offset;
}

void DrawSlotRegistry::VisRangeAllocator::Free(uint32_t offset, uint32_t count)
{
    if (count == 0)
        return;
    count = (count + 31u) & ~31u;

    FreeRange r{ offset, count };
    auto      it = std::lower_bound(
        m_free.begin(), m_free.end(), r, [](const FreeRange& a, const FreeRange& b) { return a.Offset < b.Offset; });
    auto inserted = m_free.insert(it, r);

    auto next = std::next(inserted);
    if (next != m_free.end() && inserted->Offset + inserted->Count == next->Offset)
    {
        inserted->Count += next->Count;
        m_free.erase(next);
    }
    if (inserted != m_free.begin())
    {
        auto prev = std::prev(inserted);
        if (prev->Offset + prev->Count == inserted->Offset)
        {
            prev->Count += inserted->Count;
            m_free.erase(inserted);
        }
    }
}

// ------------------------------ DrawSlotRegistry ---------------------------

void DrawSlotRegistry::Init(uint32_t maxDraws, uint32_t maxMeshletVisBits)
{
    m_maxDraws = maxDraws;
    m_visRangeAllocator.Init(maxMeshletVisBits);
    Reset();
}

void DrawSlotRegistry::Reset()
{
    m_drawSlots.clear();
    m_freeBufferIndices.clear();
    m_drawSlotHighWater = 0;
    m_visRangeAllocator.Reset();
}

void DrawSlotRegistry::BeginFrame()
{
    for (auto& kv : m_drawSlots)
        kv.second.SeenThisFrame = false;
}

DrawSlotRegistry::AcquireResult DrawSlotRegistry::Acquire(uint32_t  entityHandle,
                                                          uint32_t  meshIndex,
                                                          uint32_t  meshletCount,
                                                          uint32_t& outBufferIndex,
                                                          uint32_t& outVisOffset)
{
    auto slotIt = m_drawSlots.find(entityHandle);
    if (slotIt == m_drawSlots.end())
    {
        uint32_t bufferIndex;
        if (!m_freeBufferIndices.empty())
        {
            bufferIndex = m_freeBufferIndices.back();
            m_freeBufferIndices.pop_back();
        }
        else
        {
            if (m_drawSlotHighWater >= m_maxDraws)
            {
                YG_CORE_ERROR("DrawSlotRegistry: MeshDraw cap exceeded ({0}).", m_maxDraws);
                return AcquireResult::CapExceeded;
            }
            bufferIndex = m_drawSlotHighWater++;
        }
        slotIt                     = m_drawSlots.emplace(entityHandle, DrawSlot{}).first;
        slotIt->second.BufferIndex = bufferIndex;
    }
    DrawSlot& slot     = slotIt->second;
    slot.SeenThisFrame = true;

    if (slot.LastMeshIndex != meshIndex)
    {
        if (slot.MeshletVisCount > 0)
            m_visRangeAllocator.Free(slot.MeshletVisOffset, slot.MeshletVisCount);

        const uint32_t newOffset = m_visRangeAllocator.Alloc(meshletCount);
        if (newOffset == VisRangeAllocator::INVALID)
        {
            YG_CORE_ERROR("DrawSlotRegistry: meshlet vis bitfield exhausted (need {0} bits).", meshletCount);
            slot.MeshletVisOffset = 0;
            slot.MeshletVisCount  = 0;
            slot.LastMeshIndex    = MESH_DRAW_SENTINEL_MESH_INDEX;
            return AcquireResult::VisExhausted;
        }
        slot.MeshletVisOffset = newOffset;
        slot.MeshletVisCount  = meshletCount;
        slot.LastMeshIndex    = meshIndex;
    }

    outBufferIndex = slot.BufferIndex;
    outVisOffset   = slot.MeshletVisOffset;
    return AcquireResult::Ok;
}

void DrawSlotRegistry::CommitDraw(uint32_t entityHandle, const MeshDraw& draw, std::vector<DirtyEntry>& dirtyDraws)
{
    auto it = m_drawSlots.find(entityHandle);
    if (it == m_drawSlots.end())
        return;
    DrawSlot& slot = it->second;

    // Dirty diff: only stage if anything changed since last upload.
    if (!slot.LastUploadValid || std::memcmp(&slot.LastUploaded, &draw, sizeof(MeshDraw)) != 0)
    {
        dirtyDraws.push_back({ slot.BufferIndex, draw });
        slot.LastUploaded    = draw;
        slot.LastUploadValid = true;
    }
}

void DrawSlotRegistry::ReclaimUnseen(std::vector<DirtyEntry>& dirtyDraws)
{
    MeshDraw sentinel{};
    sentinel.MeshIndex = MESH_DRAW_SENTINEL_MESH_INDEX;

    for (auto it = m_drawSlots.begin(); it != m_drawSlots.end();)
    {
        if (!it->second.SeenThisFrame)
        {
            dirtyDraws.push_back({ it->second.BufferIndex, sentinel });
            m_freeBufferIndices.push_back(it->second.BufferIndex);
            if (it->second.MeshletVisCount > 0)
                m_visRangeAllocator.Free(it->second.MeshletVisOffset, it->second.MeshletVisCount);
            it = m_drawSlots.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace Yogi
