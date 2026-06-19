#ifndef YG_SHADER_DATA_H
#define YG_SHADER_DATA_H

#ifdef __cplusplus
#    include <cstdint>
#    include "Math/Vector.h"
#    include "Math/Matrix.h"

#    define float16_t uint16_t
#endif

// ---- Namespace (C++ only) ---------------------------------------------
#ifdef __cplusplus
namespace Yogi
{
#endif

// ---- Compile-time constants -----------------------------------------------
static const int TASK_WGSIZE                   = 32;
static const int MESH_WGSIZE                   = 64;
static const int MESHLET_MAX_VERTICES          = 64;
static const int MESHLET_MAX_TRIANGLES         = 126;
static const int CULL_WORKGROUP_SIZE           = 64;
static const int DEPTH_REDUCE_WGSIZE           = 16;
static const int MAX_TEXTURES                  = 1024;
static const int MESH_DRAW_SENTINEL_MESH_INDEX = 0xFFFFFFFFu;
static const int MAX_MATERIAL_TYPES            = 16;
static const int MAX_MESH_DRAWS                = 1000000;

// ---- Shared structs --------------------------------------------------------

struct VertexData
{
    float16_t vx, vy, vz;
    float16_t tx, ty;
    int8_t    nx, ny, nz, nw;
};

struct MeshletData
{
    uint32_t  DataOffset;
    float16_t Center[3];
    float16_t Radius;
    int8_t    ConeAxis[3];
    int8_t    ConeCutOff;
    uint8_t   TriangleCount;
    uint8_t   VertexCount;
};

struct MeshData
{
    Vector3  BoundingCenter;
    float    BoundingRadius;
    uint32_t VertexOffset;
    uint32_t MeshletDataBase;
    uint32_t MeshletOffset;
    uint32_t MeshletCount;
};

struct MeshDraw
{
    Vector3  Position;
    uint32_t MeshIndex;
    Vector4  Orientation;
    Vector3  Scale;
    uint32_t MeshletVisOffset;
    uint32_t MaterialIndex;
};

struct SceneFrame
{
    Matrix4 ProjectionViewMatrix;
    Matrix4 ViewMatrix;
    Vector4 Frustum;
    Vector2 ScreenSize;
    float   P00;
    float   P11;
    float   ZNear;
    float   ZFar;

    uint64_t VertexBuffer;
    uint64_t MeshletBuffer;
    uint64_t MeshletDataBuffer;
    uint64_t MeshDataBuffer;
    uint64_t MeshDrawBuffer;
    uint64_t VisibleDrawIndexBuffer;
    uint64_t IndirectCommandBuffer;
    uint64_t IndirectCountBuffer;
    uint64_t ObjectVisBufferRead;
    uint64_t ObjectVisBufferWrite;
    uint64_t MeshletVisBufferRead;
    uint64_t MeshletVisBufferWrite;

    uint64_t DrawIndexBuffer;
};

// Push constant for mesh/task shaders.
struct ScenePush
{
    uint64_t SceneFrameAddr;
    uint64_t MaterialBufferAddr;
    uint32_t DrawBase;
    uint32_t PyramidSlot;
};

struct CullPush
{
    uint64_t SceneFrameAddr;
    uint32_t DrawBase;
    uint32_t DrawCount;
    uint32_t PyramidSlot;
    uint32_t BucketIndex;
};

#ifdef __cplusplus
#    undef float16_t
#    undef struct
} // namespace Yogi
#endif

#endif // YG_SHADER_DATA_H
