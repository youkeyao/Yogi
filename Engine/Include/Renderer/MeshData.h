#ifdef __cplusplus
#pragma once
#include <cstdint>
namespace Yogi {
#endif

#define MESHLET_MAX_VERTICES  64
#define MESHLET_MAX_TRIANGLES 126

struct VertexData
{
#ifdef __cplusplus
    uint16_t vx, vy, vz;
    uint16_t tx, ty;
    int8_t   nx, ny, nz, nw;
    uint16_t _pad;
#else
    float16_t vx, vy, vz;
    float16_t tx, ty;
    int8_t    nx, ny, nz, nw;
    float16_t _pad;
#endif
};

struct MeshletData
{
#ifdef __cplusplus
    static constexpr uint32_t MAX_VERTICES  = MESHLET_MAX_VERTICES;
    static constexpr uint32_t MAX_TRIANGLES = MESHLET_MAX_TRIANGLES;

    uint32_t Vertices[MESHLET_MAX_VERTICES];
    uint8_t  Indices[MESHLET_MAX_TRIANGLES * 3];
    uint8_t  TriangleCount;
    uint8_t  VertexCount;
#else
    uint Vertices[MESHLET_MAX_VERTICES];
    uint8_t Indices[MESHLET_MAX_TRIANGLES * 3];
    uint8_t TriangleCount;
    uint8_t VertexCount;
#endif
};

#ifdef __cplusplus
} // namespace Yogi
#endif
