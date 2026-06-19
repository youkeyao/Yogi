import Shared.ShaderData;
import Shared.Utils;
import Materials.IMaterial;
import Materials.Standard;
import Materials.Unlit;

[[vk::push_constant]] ConstantBuffer<ScenePush> pcScene;

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
    SceneFrame sf = ((SceneFrame*)pcScene.SceneFrameAddr)[0];

    MeshDraw*  mdBuf   = (MeshDraw*)sf.MeshDrawBuffer;
    MeshData*  mDBuf   = (MeshData*)sf.MeshDataBuffer;
    MeshletData* mlBuf  = (MeshletData*)sf.MeshletBuffer;
    VertexData* vBuf    = (VertexData*)sf.VertexBuffer;
    uint*       mlDBuf  = (uint*)sf.MeshletDataBuffer;
    uint8_t*   mlD8Buf = (uint8_t*)sf.MeshletDataBuffer;
    M* matBuf  = (M*)pcScene.MaterialBufferAddr;

    MeshDraw    draw    = mdBuf[payload.drawIndex];
    MeshData    mesh    = mDBuf[draw.MeshIndex];
    uint        mi      = payload.meshletIndices[gid];
    uint        ti      = gtid;

    M m = matBuf[draw.MaterialIndex];
    MeshletData meshlet = mlBuf[mi];

    uint dataOffset    = meshlet.DataOffset + mesh.MeshletDataBase;
    uint vertexCount   = uint(meshlet.VertexCount);
    uint triangleCount = uint(meshlet.TriangleCount);

    SetMeshOutputCounts(vertexCount, triangleCount);

    // Outline thickness in world units. Could become a material attribute
    // later; for the demo a single scalar is enough.
    const float kOutlineThickness = 0.03f;

    for (uint i = ti; i < vertexCount; i += MESH_WGSIZE)
    {
        uint vi = mlDBuf[dataOffset + i] + mesh.VertexOffset;
        VertexData v = vBuf[vi];
        float3 localPosition = float3(v.vx, v.vy, v.vz) * draw.Scale;
        float3 worldPos = RotateByQuaternion(localPosition, draw.Orientation) + draw.Position;
        float3 normal   = float3(v.nx, v.ny, v.nz) / 127.0;
        float3 worldNormal = normalize(RotateByQuaternion(normal, draw.Orientation));

        // Same ShadingContext shape as the main mesh stage, so the material's
        // vertexHook sees consistent inputs across passes.
        ShadingContext ctx;
        ctx.worldPos    = worldPos;
        ctx.worldNormal = worldNormal;
        ctx.viewDir     = float3(0, 0, 1);
        ctx.uv0         = float2(v.tx, v.ty);
        worldPos = m.vertexHook(worldPos, ctx);

        // Inflate along world-space normal AFTER the material's vertex
        // hook -- so wind/water displacement happens on the un-inflated
        // surface and the outline grows uniformly from the displaced shell.
        worldPos += worldNormal * kOutlineThickness;

        float4 clipPos  = mul(sf.ProjectionViewMatrix, float4(worldPos, 1.0));

        MeshletVertex out_v;
        out_v.Position      = clipPos;
        out_v.NormalShade   = float3(0, 0, 0); // unused by outline fragment
        out_v.UV            = float2(v.tx, v.ty);
        out_v.MaterialIndex = draw.MaterialIndex;
        verts[i] = out_v;
    }

    for (uint i = ti; i < triangleCount; i += MESH_WGSIZE)
    {
        uint offset = (dataOffset + vertexCount) * 4 + i * 3;
        uint a = uint(mlD8Buf[offset]), b = uint(mlD8Buf[offset + 1]), c = uint(mlD8Buf[offset + 2]);
        prims[i] = uint3(a, b, c);

        // No primitive culling: the pipeline-state cull mode (CullMode::Front)
        // is what selects the back faces of the inflated shell. Outline
        // doesn't run the sub-pixel triangle cull from the main pass --
        // outlines on tiny triangles are perceptually fine.
        MeshletPrimitiveAttrs pa_out;
        pa_out.CullPrimitive = false;
        primAttrs[i] = pa_out;
    }
}
