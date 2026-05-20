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

struct SceneData
{
    mat4 ProjectionViewMatrix;
    mat4 ViewMatrix;
    vec2 ScreenSize; // pixel dimensions of the render target; used by triangle sub-pixel culling
    uint DrawBase;
    uint _Pad0;
};

struct CullData
{
    mat4 View; // world -> view (GLM convention, -Z forward)

    // niagara-style symmetric frustum: (Lx, Lz, Ty, Tz) of normalized L/T plane normals
    // expressed in view space. Right/Bottom planes are mirrored via abs() in the cull
    // shader. 4 floats replace 6 frustum planes (saves 80 bytes of push constant).
    vec4 Frustum;

    // P00, P11: projection diagonal entries (tan-half-fov reciprocals).
    // ZNear, ZFar: positive view-space distances. Used for the perspectiveRH_ZO depth
    //   formula z_ndc = far * (d - near) / (d * (far - near)).
    float P00;
    float P11;
    float ZNear;
    float ZFar;

    uint DrawBase;
    uint DrawCount;
    uint OutputBase;
    uint CountIndex;
    uint IsLate; // 0 = EARLY pass (gate on prev visibility), 1 = LATE pass (Hi-Z + write visibility)

    // Pad to 128 bytes (Vulkan-guaranteed minimum maxPushConstantsSize).
    uint _Pad0;
    uint _Pad1;
    uint _Pad2;
};

struct MeshDraw
{
    vec3 Position;
    uint MeshIndex;
    vec4 Orientation;
    vec3 Scale;
    uint _Pad0;
};

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