import Shared.ShaderData;
import Shared.Utils;
import Shared.Bindless;
import Materials.IMaterial;
import Materials.Standard;
import Materials.Unlit;
#include "Renderer/Passes/MeshletDrawPush.h"

#define TRIANGLE_CULL 1

[[vk::push_constant]] ConstantBuffer<MeshletDrawPush> pcMeshletDraw;

#if TRIANGLE_CULL
groupshared float4 s_clipPos[MESHLET_MAX_VERTICES];
#endif

[shader("mesh")]
[outputtopology("triangle")]
[numthreads(MESH_WGSIZE, 1, 1)]
void msMain<M : IMaterial>(uint gid  : SV_GroupID,
                           uint gtid : SV_GroupIndex,
                           in payload TaskPayload payload,
                           out vertices MeshletVertex verts[MESHLET_MAX_VERTICES],
                           out indices uint3 prims[MESHLET_MAX_TRIANGLES],
                           out primitives MeshletPrimitiveAttrs primAttrs[MESHLET_MAX_TRIANGLES])
{
    SceneFrame sf = ((SceneFrame*)pcMeshletDraw.SceneFrameAddr)[0];

    MeshDraw*  mdBuf   = (MeshDraw*)pcMeshletDraw.MeshDrawBuffer;
    MeshData*  mDBuf   = (MeshData*)pcMeshletDraw.MeshDataBuffer;
    MeshletData* mlBuf  = (MeshletData*)pcMeshletDraw.MeshletBuffer;
    VertexData* vBuf    = (VertexData*)pcMeshletDraw.VertexBuffer;
    uint*       mlDBuf  = (uint*)pcMeshletDraw.MeshletDataBuffer;
    uint8_t*   mlD8Buf = (uint8_t*)pcMeshletDraw.MeshletDataBuffer;
    M* matBuf  = (M*)pcMeshletDraw.MaterialBufferAddr;

    MeshDraw draw = mdBuf[payload.drawIndex];
    MeshData mesh = mDBuf[draw.MeshIndex];
    uint mi = payload.meshletIndices[gid];
    uint ti = gtid;

    M m = matBuf[draw.MaterialIndex];

    MeshletData meshlet = mlBuf[mi];
    uint dataOffset    = meshlet.DataOffset + mesh.MeshletDataBase;
    uint vertexCount   = uint(meshlet.VertexCount);
    uint triangleCount = uint(meshlet.TriangleCount);

    SetMeshOutputCounts(vertexCount, triangleCount);

    for (uint i = ti; i < vertexCount; i += MESH_WGSIZE)
    {
        uint vi = mlDBuf[dataOffset + i] + mesh.VertexOffset;
        VertexData v = vBuf[vi];
        float3 localPosition = float3(v.vx, v.vy, v.vz) * draw.Scale;
        float3 worldPos = RotateByQuaternion(localPosition, draw.Orientation) + draw.Position;
        float3 normal   = float3(v.nx, v.ny, v.nz) / 127.0;

        ShadingContext ctx;
        ctx.worldPos    = worldPos;
        ctx.worldNormal = normal;
        ctx.viewDir     = float3(0, 0, 1);
        ctx.uv0         = float2(v.tx, v.ty);
        worldPos = m.vertexHook(worldPos, ctx);

        float4 clipPos  = mul(sf.ProjectionViewMatrix, float4(worldPos, 1.0));

        MeshletVertex out_v;
        out_v.Position       = clipPos;
        out_v.NormalShade    = normal * 0.5 + 0.5;
        out_v.UV             = float2(v.tx, v.ty);
        out_v.MaterialIndex  = draw.MaterialIndex;
        verts[i] = out_v;

#if TRIANGLE_CULL
        s_clipPos[i] = clipPos;
#endif
    }

#if TRIANGLE_CULL
    GroupMemoryBarrierWithGroupSync();
#endif

    for (uint i = ti; i < triangleCount; i += MESH_WGSIZE)
    {
        uint offset = (dataOffset + vertexCount) * 4 + i * 3;
        uint a = uint(mlD8Buf[offset]), b = uint(mlD8Buf[offset + 1]), c = uint(mlD8Buf[offset + 2]);
        prims[i] = uint3(a, b, c);

#if TRIANGLE_CULL
        // Cull only when all three vertices are in front of the near plane.
        float4 va = s_clipPos[a];
        float4 vb = s_clipPos[b];
        float4 vc = s_clipPos[c];
        bool culled = false;
        if (va.w > 0.0 && vb.w > 0.0 && vc.w > 0.0)
        {
            float2 pa = va.xy / va.w;
            float2 pb = vb.xy / vb.w;
            float2 pc = vc.xy / vc.w;

            float2 eb = pb - pa;
            float2 ec = pc - pa;
            culled = culled || (eb.x * ec.y <= eb.y * ec.x);

            float2 ssa = (pa * 0.5 + 0.5) * sf.ScreenSize;
            float2 ssb = (pb * 0.5 + 0.5) * sf.ScreenSize;
            float2 ssc = (pc * 0.5 + 0.5) * sf.ScreenSize;
            float2 bmin = min(ssa, min(ssb, ssc));
            float2 bmax = max(ssa, max(ssb, ssc));
            const float sbprec = 1.0 / 256.0;
            culled = culled || (round(bmin.x - sbprec) == round(bmax.x))
                            || (round(bmin.y - sbprec) == round(bmax.y));
        }
        MeshletPrimitiveAttrs pa_out;
        pa_out.CullPrimitive = culled;
        primAttrs[i] = pa_out;
#else
        MeshletPrimitiveAttrs pa_out;
        pa_out.CullPrimitive = false;
        primAttrs[i] = pa_out;
#endif
    }
}
