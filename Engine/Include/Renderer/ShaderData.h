#ifdef __cplusplus
#    pragma once
#    include <cstdint>
#    include "Math/Matrix.h"
#    define uint      uint32_t
#    define float16_t uint16_t
#    define vec3      Yogi::Vector3
#    define vec4      Yogi::Vector4
#    define mat4      Yogi::Matrix4
#else
#    define CULL        1
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
    float16_t Center[3];
    float16_t Radius;
    int8_t    ConeAxis[3];
    int8_t    ConeCutOff;
    uint      DataOffset;
    uint8_t   TriangleCount;
    uint8_t   VertexCount;
};

struct SceneData
{
    mat4 ProjectionViewMatrix;
    mat4 ViewMatrix;
};

struct MeshDraw
{
    vec3  Position;
    float Scale;
    vec4  Orientation;

    uint MeshletOffset;
    uint MeshletCount;
    uint VertexOffset;
    uint _pad0;
};

#ifdef __cplusplus
#    undef uint
#    undef float16_t
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