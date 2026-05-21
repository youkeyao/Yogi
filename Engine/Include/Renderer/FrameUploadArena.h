#pragma once

#include "Renderer/RHI/IBuffer.h"

namespace Yogi
{

// Per-frame linear/bump upload arena.
//
// Replaces the "one mapped IBuffer per frame slot per per-frame struct" pattern
// with a single host-visible IBuffer carved into N segments (N = frames-in-flight).
// Within the current frame's segment a bump pointer hands out small allocations
// for SceneFrame / CullFrame / etc., returning a GPU device address (BDA) that
// callers stuff into push constants.
//
// Contract:
//   - BeginFrame(slot) is called once per frame before any Push().
//   - The caller must guarantee that the frame slot's previous use is complete
//     before BeginFrame() resets m_head (same guarantee the previous per-slot
//     buffers relied on -- typically a commandBuffer->Wait() on the slot).
//   - Push() never grows the buffer; assert-fires if the segment is exhausted.
class YG_API FrameUploadArena
{
public:
    FrameUploadArena() = default;

    // segmentSize  = bytes reserved per frame slot.
    // segmentCount = frames-in-flight (== ISwapChain::GetImageCount()).
    void Init(uint64_t segmentSize, uint32_t segmentCount);
    void Shutdown();

    // Switch to slot's segment and reset its bump pointer. Caller is responsible
    // for ensuring the previous use of this slot has completed on the GPU.
    void BeginFrame(uint32_t slot);

    // Copy `size` bytes of `data` into the current frame's segment at the next
    // aligned offset and return the GPU device address of that region.
    //
    // `alignment` defaults to 16 -- enough for std430 vec4 / mat4 / u64 BDA loads
    // referenced through buffer_reference. Callers needing tighter alignment
    // (e.g. 256-byte UBOs) can override.
    uint64_t Push(const void* data, uint64_t size, uint64_t alignment = 16);

private:
    static constexpr uint64_t k_segmentAlign = 256; // safe for any per-frame struct's natural alignment

    static uint64_t AlignUp(uint64_t v, uint64_t a) { return (v + a - 1) & ~(a - 1); }

    WRef<IBuffer> m_buffer;
    uint64_t      m_baseAddress  = 0;
    uint64_t      m_segmentSize  = 0;
    uint32_t      m_segmentCount = 0;
    uint32_t      m_currentSlot  = 0;
    uint64_t      m_head         = 0;
};

} // namespace Yogi
