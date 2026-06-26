#pragma once

#ifdef __cplusplus
#    include <cstdint>
namespace Yogi
{
#endif

struct ObjectCullPush
{
    uint64_t SceneFrameAddr;
    uint64_t VisibleDrawIndexBuffer;
    uint64_t IndirectCommandBuffer;
    uint64_t IndirectCountBuffer;
    uint64_t ObjectVisBufferRead;
    uint64_t ObjectVisBufferWrite;
    uint64_t MeshDataBuffer;
    uint64_t MeshDrawBuffer;
    uint64_t DrawIndexBuffer;
    uint32_t DrawBase;
    uint32_t DrawCount;
    uint32_t PyramidSlot;
    uint32_t BucketIndex;
};

#ifdef __cplusplus
} // namespace Yogi
#endif
