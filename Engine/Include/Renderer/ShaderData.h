#ifndef YG_SHADER_DATA_H
#define YG_SHADER_DATA_H

#ifdef __cplusplus
#    pragma once
#    include <cstdint>
#    include "Math/Matrix.h"
#    define uint          uint32_t
#    define float16_t     uint16_t
#    define vec2          Yogi::Vector2
#    define vec3          Yogi::Vector3
#    define vec4          Yogi::Vector4
#    define mat4          Yogi::Matrix4
#    define YG_GPU_STRUCT alignas(16)
#else
#    define YG_GPU_STRUCT
#    define TASK_CULL     1
#    define TRIANGLE_CULL 1
#    define MESH_WGSIZE   32
#endif

#define TASK_WGSIZE           32
#define MESHLET_MAX_VERTICES  64
#define MESHLET_MAX_TRIANGLES 126
#define CULL_WORKGROUP_SIZE   64
#define MAX_TEXTURES          1024

struct VertexData
{
    float16_t vx, vy, vz;
    float16_t tx, ty;
    int8_t    nx, ny, nz, nw;
    float16_t _pad;
};

struct MeshletData
{
    float16_t Center[3];
    float16_t Radius;
    int8_t    ConeAxis[3];
    int8_t    ConeCutOff;
    uint      DataOffset;
    uint8_t   TriangleCount;
    uint8_t   VertexCount;
};

struct YG_GPU_STRUCT MeshData
{
    vec3  BoundingCenter;
    float BoundingRadius;
    uint  VertexOffset;
    uint  MeshletDataBase;
    uint  MeshletOffset;
    uint  MeshletCount;
};

struct YG_GPU_STRUCT MeshDraw
{
    vec3 Position;
    uint MeshIndex;
    vec4 Orientation;
    vec3 Scale;
    uint MeshletVisOffset;
    uint MaterialIndex;
};

struct YG_GPU_STRUCT MaterialData
{
    vec4 BaseColor;
    uint AlbedoTexture;
    uint _Pad0;
    uint _Pad1;
    uint _Pad2;
};

struct YG_GPU_STRUCT SceneFrame
{
    mat4  ProjectionViewMatrix; // off   0, size 64
    mat4  ViewMatrix;           // off  64, size 64
    vec4  Frustum;              // off 128, size 16
    vec2  ScreenSize;           // off 144, size  8  -- triangle sub-pixel cull
    float P00;                  // off 152
    float P11;                  // off 156
    float ZNear;                // off 160
    float ZFar;                 // off 164

    uint64_t VertexBuffer;           // off 168
    uint64_t MeshletBuffer;          // off 176
    uint64_t MeshletDataBuffer;      // off 184 (uint8 view via cast in shader)
    uint64_t MeshDataBuffer;         // off 192
    uint64_t MeshDrawBuffer;         // off 200
    uint64_t VisibleDrawIndexBuffer; // off 208
    uint64_t IndirectCommandBuffer;  // off 216 (cull writes; indirect dispatch reads)
    uint64_t IndirectCountBuffer;    // off 224
    uint64_t ObjectVisBuffer;        // off 232 (per-MeshDraw two-phase visibility)
    uint64_t MeshletVisBuffer;       // off 240 (per-instance per-meshlet vis)
    uint64_t MaterialBuffer;         // off 248 (per-unique-material data, bindless)
}; // total 256

// Push constant for mesh/task shaders. Carries only the SceneFrame buffer
// address + per-batch DrawBase; matrices/buffer pointers come via SceneFrame.
struct ScenePush
{
    uint64_t SceneFrameAddr; // off  0, 8
    uint     DrawBase;       // off  8, 4
    uint     PyramidSlot;    // off 12, 4  Hi-Z pyramid sampled slot (LATE task only; EARLY ignores)
}; // total 16

struct CullPush
{
    uint64_t SceneFrameAddr; // off  0, 8
    uint     DrawBase;       // off  8, 4
    uint     DrawCount;      // off 12, 4
    uint     OutputBase;     // off 16, 4
    uint     CountIndex;     // off 20, 4
    uint     PyramidSlot;    // off 24, 4  Hi-Z pyramid sampled slot (LATE only)
    uint     _Pad0;          // off 28, 4
}; // total 32

#ifdef __cplusplus
#    undef uint
#    undef float16_t
#    undef vec2
#    undef vec3
#    undef vec4
#    undef mat4
#    undef YG_GPU_STRUCT
#else
struct TaskPayload
{
    uint meshletIndices[TASK_WGSIZE];
    uint drawIndex;
};

vec3 RotateByQuaternion(vec3 v, vec4 q)
{
    vec3 t = 2.0 * cross(q.xyz, v);
    return v + q.w * t + cross(q.xyz, t);
}
#endif

#endif