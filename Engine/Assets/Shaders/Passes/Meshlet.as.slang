import Shared.ShaderData;
import Shared.Utils;
import Shared.Bindless;
#include "Renderer/Passes/MeshletDrawPush.h"

#define TASK_CULL 1

[[vk::push_constant]] ConstantBuffer<MeshletDrawPush> pcMeshletDraw;

bool coneCull(float3 center, float radius, float3 coneAxis, float coneCutoff, float3 cameraPosition)
{
    return dot(center - cameraPosition, coneAxis) > coneCutoff * length(center - cameraPosition) + radius;
}

groupshared uint        s_survivorCount;
groupshared TaskPayload s_payload;

[shader("amplification")]
[numthreads(TASK_WGSIZE, 1, 1)]
void asMain(uint  gid    : SV_GroupID,
            uint  gtid   : SV_GroupIndex,
            uint  drawId : SV_DrawIndex)
{
    SceneFrame sf = ((SceneFrame*)pcMeshletDraw.SceneFrameAddr)[0];

    uint*     vdiBuf = (uint*)pcMeshletDraw.VisibleDrawIndexBuffer;
    MeshDraw* mdBuf  = (MeshDraw*)pcMeshletDraw.MeshDrawBuffer;
    MeshData* mDBuf  = (MeshData*)pcMeshletDraw.MeshDataBuffer;
    MeshletData* mlBuf  = (MeshletData*)pcMeshletDraw.MeshletBuffer;

    uint drawIndex = vdiBuf[drawId + pcMeshletDraw.DrawBase];
    if (gtid == 0)
        s_payload.drawIndex = drawIndex;

    MeshDraw draw = mdBuf[drawIndex];
    MeshData mesh = mDBuf[draw.MeshIndex];
    uint ti = gtid;
    uint miLocal = gid * TASK_WGSIZE + ti;

#if TASK_CULL
    if (ti == 0)
        s_survivorCount = 0;
    GroupMemoryBarrierWithGroupSync();

    if (miLocal < mesh.MeshletCount)
    {
        uint mi      = mesh.MeshletOffset + miLocal;
        uint visIdx  = draw.MeshletVisOffset + miLocal;
        bool prevMlVisible = VisBitTest(pcMeshletDraw.MeshletVisBufferRead, visIdx);
        MeshletData meshlet = mlBuf[mi];

        // World-space sphere center, then to view (+Z forward like ObjectCull).
        float3 center = float3(meshlet.Center[0], meshlet.Center[1], meshlet.Center[2]);
        center = RotateByQuaternion(center * draw.Scale, draw.Orientation) + draw.Position;
        float3 viewCenter = mul(sf.ViewMatrix, float4(center, 1.0)).xyz;
        viewCenter.z = -viewCenter.z;
        float maxScale = max(max(abs(draw.Scale.x), abs(draw.Scale.y)), abs(draw.Scale.z));
        float radius = float(meshlet.Radius) * maxScale;

        // Cone cull (camera at origin in view space; coneAxis flipped to match Z).
        float3 coneAxis = float3(meshlet.ConeAxis[0], meshlet.ConeAxis[1], meshlet.ConeAxis[2]) / 127.0;
        coneAxis = mul((float3x3)sf.ViewMatrix, RotateByQuaternion(coneAxis, draw.Orientation));
        coneAxis.z = -coneAxis.z;
        bool visible = !coneCull(viewCenter, radius, coneAxis, meshlet.ConeCutOff / 127.0, float3(0, 0, 0));

        // Frustum cull -- runs in both EARLY and LATE since it doesn't depend
        // on a pyramid; just static this-frame projection planes.
        visible = visible && sphereInsideFrustum(viewCenter, radius, sf.Frustum, sf.ZNear, sf.ZFar);

#ifdef LATE
        // Hi-Z occlusion (LATE only).
        if (visible)
        {
            float4 aabb;
            if (projectSphere(viewCenter, radius, sf.ZNear, sf.P00, sf.P11, aabb))
            {
                float w     = aabb.z - aabb.x;
                float h     = aabb.w - aabb.y;
                uint2 p0    = BindlessTextureSize(pcMeshletDraw.PyramidSlot);
                float level = ceil(log2(max(w * float(p0.x), h * float(p0.y))));
                float depth = SampleBindlessMaxLod(pcMeshletDraw.PyramidSlot,
                                                   (aabb.xy + aabb.zw) * 0.5,
                                                   level).r;

                float closestZ    = viewCenter.z - radius;
                float depthSphere = sf.ZFar * (closestZ - sf.ZNear)
                                  / (closestZ * (sf.ZFar - sf.ZNear));
                visible = depthSphere <= depth;
            }
        }

        if (visible)
            VisBitSet(pcMeshletDraw.MeshletVisBufferWrite, visIdx);

        if (visible && !prevMlVisible)
        {
            uint idx;
            InterlockedAdd(s_survivorCount, 1, idx);
            s_payload.meshletIndices[idx] = mi;
        }
#else
        // Write visibility for next frame regardless of prevMlVisible,
        // so the Early/Late decision stays stable across frames.
        if (visible)
            VisBitSet(pcMeshletDraw.MeshletVisBufferWrite, visIdx);

        if (visible && prevMlVisible)
        {
            uint idx;
            InterlockedAdd(s_survivorCount, 1, idx);
            s_payload.meshletIndices[idx] = mi;
        }
#endif
    }
    GroupMemoryBarrierWithGroupSync();

    DispatchMesh(s_survivorCount, 1, 1, s_payload);
#else
    // TASK_CULL disabled: pass ALL meshlets to mesh shader.
    if (miLocal < mesh.MeshletCount)
        s_payload.meshletIndices[ti] = mesh.MeshletOffset + miLocal;

    GroupMemoryBarrierWithGroupSync();

    uint numMeshGroups = (mesh.MeshletCount + TASK_WGSIZE - 1) / TASK_WGSIZE;
    DispatchMesh(numMeshGroups, 1, 1, s_payload);
#endif
}
