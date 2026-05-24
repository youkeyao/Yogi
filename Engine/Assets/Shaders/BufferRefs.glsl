#ifndef YOGI_BUFFER_REFS_GLSL
#define YOGI_BUFFER_REFS_GLSL

#extension GL_EXT_buffer_reference       : require
#extension GL_EXT_buffer_reference_uvec2 : require

#include "Renderer/ShaderData.h"

layout(buffer_reference, std430) readonly buffer VertexBufferRef       { VertexData  v[]; };
layout(buffer_reference, std430) readonly buffer MeshletBufferRef      { MeshletData m[]; };
layout(buffer_reference, std430) readonly buffer MeshletDataBufferRef  { uint        d[]; };
layout(buffer_reference, std430) readonly buffer MeshletData8BufferRef { uint8_t     d[]; };
layout(buffer_reference, std430) readonly buffer MeshDataBufferRef     { MeshData    m[]; };
layout(buffer_reference, std430) readonly buffer MeshDrawBufferRef     { MeshDraw    m[]; };
layout(buffer_reference, std430)          buffer VisibleDrawIdxBufRef  { uint        i[]; };
layout(buffer_reference, std430)          buffer IndirectCmdBufRef     { uint        c[]; };
layout(buffer_reference, std430)          buffer IndirectCountBufRef   { uint        n[]; };
layout(buffer_reference, std430)          buffer ObjectVisBufRef       { uint        v[]; };
layout(buffer_reference, std430)          buffer MeshletVisBufRef       { uint        v[]; };
layout(buffer_reference, std430) readonly buffer SceneFrameRef         { SceneFrame fd; };

bool sphereInsideFrustum(vec3 c, float r, vec4 frustum, float zNear, float zFar)
{
    bool visible = true;
    visible = visible && c.z * frustum.y - abs(c.x) * frustum.x > -r;
    visible = visible && c.z * frustum.w - abs(c.y) * frustum.z > -r;
    visible = visible && c.z + r > zNear && c.z - r < zFar;
    return visible;
}

bool projectSphere(vec3 c, float r, float znear, float P00, float P11, out vec4 aabb)
{
    if (c.z < r + znear)
        return false;

    vec3  cr   = c * r;
    float czr2 = c.z * c.z - r * r;

    float vx   = sqrt(c.x * c.x + czr2);
    float minx = (vx * c.x - cr.z) / (vx * c.z + cr.x);
    float maxx = (vx * c.x + cr.z) / (vx * c.z - cr.x);

    float vy   = sqrt(c.y * c.y + czr2);
    float miny = (vy * c.y - cr.z) / (vy * c.z + cr.y);
    float maxy = (vy * c.y + cr.z) / (vy * c.z - cr.y);

    aabb = vec4(minx * P00, miny * P11, maxx * P00, maxy * P11);
    aabb = aabb.xwzy * vec4(0.5, -0.5, 0.5, -0.5) + vec4(0.5);
    return true;
}

#endif