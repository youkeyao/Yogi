#ifndef YG_SHADER_DATA_H
#define YG_SHADER_DATA_H

#ifdef __cplusplus
#    pragma once
#    include <cstdint>
#    include "Math/Matrix.h"
#    define uint      uint32_t
#    define float16_t uint16_t
#    define vec2      Yogi::Vector2
#    define vec3      Yogi::Vector3
#    define vec4      Yogi::Vector4
#    define mat4      Yogi::Matrix4
#else
#    define TASK_CULL     1
#    define TRIANGLE_CULL 1
#    define MESH_WGSIZE   32
#endif

#define TASK_WGSIZE           32
#define MESHLET_MAX_VERTICES  64
#define MESHLET_MAX_TRIANGLES 126
#define CULL_WORKGROUP_SIZE   64

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

struct MeshData
{
    vec3  BoundingCenter;
    float BoundingRadius;
    uint  VertexOffset;
    uint  MeshletDataBase;
    uint  MeshletOffset;
    uint  MeshletCount;
};

struct MeshDraw
{
    vec3 Position;
    uint MeshIndex;
    vec4 Orientation;
    vec3 Scale;
    uint _Pad0;
};

// ---------------------------------------------------------------------------
// SceneFrame / CullFrame: per-frame static data, uploaded once per frame to a
// Storage buffer and dereferenced via buffer_reference (BDA). Keeps the push
// constants small (16 B / 32 B) so we fit the Vulkan-guaranteed 128 B max.
// ---------------------------------------------------------------------------

struct SceneFrame
{
    mat4 ProjectionViewMatrix; // off   0, size 64
    mat4 ViewMatrix;           // off  64, size 64
    vec2 ScreenSize;           // off 128, size  8 — used by triangle sub-pixel cull
    uint _Pad0;                // off 136
    uint _Pad1;                // off 140 — 8-byte align for u64 block

    uint64_t VertexBuffer;           // off 144
    uint64_t MeshletBuffer;          // off 152
    uint64_t MeshletDataBuffer;      // off 160 (uint8 view via cast in shader)
    uint64_t MeshDataBuffer;         // off 168
    uint64_t MeshDrawBuffer;         // off 176
    uint64_t VisibleDrawIndexBuffer; // off 184
}; // total 192

// Push constant for mesh/task shaders. Carries only the SceneFrame buffer
// address + per-batch DrawBase; matrices/buffer pointers come via SceneFrame.
struct ScenePush
{
    uint64_t SceneFrameAddr; // off  0, 8
    uint     DrawBase;       // off  8, 4
    uint     _Pad0;          // off 12, 4
}; // total 16

struct CullFrame
{
    mat4 View; // world -> view (GLM convention, -Z forward)               // off  0, 64

    // niagara-style symmetric frustum: (Lx, Lz, Ty, Tz) of normalized L/T plane normals
    // expressed in view space. Right/Bottom planes are mirrored via abs() in the cull
    // shader. 4 floats replace 6 frustum planes (saves 80 bytes vs full 6-plane form).
    vec4 Frustum; // off 64, 16

    // P00, P11: projection diagonal entries (1/tan(fovY/2)/aspect, 1/tan(fovY/2)).
    // ZNear, ZFar: positive view-space distances. Used for the perspectiveRH_ZO depth
    //   formula z_ndc = far * (d - near) / (d * (far - near)).
    float P00;   // off 80
    float P11;   // off 84
    float ZNear; // off 88
    float ZFar;  // off 92

    uint64_t MeshDataBuffer;         // off 96
    uint64_t MeshDrawBuffer;         // off 104
    uint64_t IndirectCommandBuffer;  // off 112
    uint64_t VisibleDrawIndexBuffer; // off 120
    uint64_t IndirectCountBuffer;    // off 128
    uint64_t VisibilityBuffer;       // off 136
}; // total 144

// Push constant for ObjectCull.comp. Carries the CullFrame buffer address +
// per-dispatch state (which batch we're processing, EARLY vs LATE).
struct CullPush
{
    uint64_t CullFrameAddr; // off  0, 8
    uint     DrawBase;      // off  8, 4
    uint     DrawCount;     // off 12, 4
    uint     OutputBase;    // off 16, 4
    uint     CountIndex;    // off 20, 4
    uint     IsLate;        // 0 = EARLY (gate on prev visibility); 1 = LATE (Hi-Z + write visibility)
                            // off 24, 4
    uint _Pad0;             // off 28, 4
}; // total 32

#ifdef __cplusplus
#    undef uint
#    undef float16_t
#    undef vec2
#    undef vec3
#    undef vec4
#    undef mat4
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