#include "Renderer/FrameUploadArena.h"
#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

void FrameUploadArena::Init(uint64_t segmentSize, uint32_t segmentCount)
{
    YG_CORE_ASSERT(segmentCount > 0, "FrameUploadArena: segmentCount must be > 0");
    m_segmentSize  = AlignUp(segmentSize, k_segmentAlign);
    m_segmentCount = segmentCount;
    m_buffer       = ResourceManager::CreateResource<IBuffer>(
        BufferDesc{ m_segmentSize * m_segmentCount, BufferUsage::Storage, BufferAccess::Dynamic });
    m_baseAddress = m_buffer->GetDeviceAddress();
    m_currentSlot = 0;
    m_head        = 0;
}

void FrameUploadArena::Shutdown()
{
    m_buffer       = nullptr;
    m_baseAddress  = 0;
    m_segmentSize  = 0;
    m_segmentCount = 0;
    m_currentSlot  = 0;
    m_head         = 0;
}

void FrameUploadArena::BeginFrame(uint32_t slot)
{
    YG_CORE_ASSERT(m_buffer && slot < m_segmentCount,
                   "FrameUploadArena: BeginFrame called before Init / bad slot");
    m_currentSlot = slot;
    m_head        = 0;
}

uint64_t FrameUploadArena::Push(const void* data, uint64_t size, uint64_t alignment)
{
    YG_CORE_ASSERT(m_buffer, "FrameUploadArena: Push called before Init");
    const uint64_t segmentBase = static_cast<uint64_t>(m_currentSlot) * m_segmentSize;
    const uint64_t offsetInSeg = AlignUp(m_head, alignment);
    YG_CORE_ASSERT(offsetInSeg + size <= m_segmentSize,
                   "FrameUploadArena: segment exhausted -- raise segmentSize at Init()");

    m_buffer->UpdateData(data, size, segmentBase + offsetInSeg);
    m_head = offsetInSeg + size;
    return m_baseAddress + segmentBase + offsetInSeg;
}

} // namespace Yogi
