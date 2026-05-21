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
layout(buffer_reference, std430)          buffer VisibilityBufRef      { uint        v[]; };

layout(buffer_reference, std430) readonly buffer SceneFrameRef         { SceneFrame fd; };
layout(buffer_reference, std430) readonly buffer CullFrameRef          { CullFrame  fd; };

#endif