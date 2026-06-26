#pragma once

#ifdef __cplusplus
#    include <cstdint>
namespace Yogi
{
#endif

struct MeshletDrawPush
{
    uint64_t SceneFrameAddr;
    uint64_t MaterialBufferAddr;
    uint64_t VisibleDrawIndexBuffer;
    uint64_t MeshletVisBufferRead;
    uint64_t MeshletVisBufferWrite;
    uint64_t VertexBuffer;
    uint64_t MeshletBuffer;
    uint64_t MeshletDataBuffer;
    uint64_t MeshDataBuffer;
    uint64_t MeshDrawBuffer;
    uint32_t DrawBase;
    uint32_t PyramidSlot;
};

#ifdef __cplusplus
} // namespace Yogi
#endif
