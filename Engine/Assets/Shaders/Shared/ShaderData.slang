#define Vector2 float2
#define Vector3 float3
#define Vector4 float4
#define Matrix4 float4x4

#include "Renderer/ShaderData.h"

// ---- Slang-only helpers (not in the C++ header) ---------------------

// Task shader payload passed from amplification shader to mesh shader.
struct TaskPayload
{
    uint drawIndex;
    uint meshletIndices[TASK_WGSIZE];
};

struct MeshletVertex
{
    float4 Position : SV_Position;
    float3 NormalShade;
    float2 UV;
    nointerpolation uint MaterialIndex;
};

struct MeshletPrimitiveAttrs
{
    bool CullPrimitive : SV_CullPrimitive;
};