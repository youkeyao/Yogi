#ifdef __cplusplus
#    pragma once
#    include <cstdint>
#    include "Math/Matrix.h"
#    define uint      uint32_t
#    define int8_t    int8_t
#    define uint8_t   uint8_t
#    define float16_t uint16_t
#    define mat4      Yogi::Matrix4
#else
#    define CULL        0
#    define MESH_WGSIZE 32
#endif

#define TASK_WGSIZE           32
#define MESHLET_MAX_VERTICES  64
#define MESHLET_MAX_TRIANGLES 126

struct VertexData
{
    float16_t vx, vy, vz;
    float16_t tx, ty;
    int8_t    nx, ny, nz, nw;
    float16_t _pad;
};

struct MeshletData
{
    float   Cone[4];
    uint    DataOffset;
    uint8_t TriangleCount;
    uint8_t VertexCount;
};

struct SceneData
{
    mat4 ProjectionViewMatrix;
};

#ifdef __cplusplus
#    undef uint
#    undef float16_t
#else
struct TaskPayload
{
    uint meshletIndices[TASK_WGSIZE];
};
#endif