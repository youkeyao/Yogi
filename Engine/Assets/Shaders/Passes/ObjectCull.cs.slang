import Shared.ShaderData;
import Shared.Utils;

#ifdef LATE
import Shared.Bindless;
#endif

[[vk::push_constant]] ConstantBuffer<CullPush> pcCull;

[shader("compute")]
[numthreads(CULL_WORKGROUP_SIZE, 1, 1)]
void csMain(uint3 dtid : SV_DispatchThreadID)
{
    uint drawIndex = dtid.x;
    if (drawIndex >= pcCull.DrawCount)
        return;

    SceneFrame sf = ((SceneFrame*)pcCull.SceneFrameAddr)[0];

    uint* drawIndexBuf = (uint*)sf.DrawIndexBuffer;
    uint  meshDrawIndex = drawIndexBuf[pcCull.DrawBase + drawIndex];

    MeshDraw* mdBuf     = (MeshDraw*)sf.MeshDrawBuffer;
    MeshData* mDBuf     = (MeshData*)sf.MeshDataBuffer;
    uint*     icBuf      = (uint*)sf.IndirectCommandBuffer;
    uint*     vdiBuf     = (uint*)sf.VisibleDrawIndexBuffer;
    uint*     icountBuf  = (uint*)sf.IndirectCountBuffer;

#ifndef LATE
    // EARLY: skip objects invisible last frame (LATE will catch them).
    if (!VisBitTest(sf.ObjectVisBufferRead, meshDrawIndex))
        return;
#endif

    MeshDraw draw = mdBuf[meshDrawIndex];

    if (draw.MeshIndex == MESH_DRAW_SENTINEL_MESH_INDEX)
        return;

    MeshData mesh = mDBuf[draw.MeshIndex];

    // World-space sphere center: matches the vertex shader's TRS.
    float3 center = RotateByQuaternion(mesh.BoundingCenter * draw.Scale, draw.Orientation) + draw.Position;
    float maxScale = max(max(abs(draw.Scale.x), abs(draw.Scale.y)), abs(draw.Scale.z));
    float radius = mesh.BoundingRadius * maxScale;

    // GLM (-Z forward) -> niagara view-space (+Z forward).
    float3 viewPos = mul(sf.ViewMatrix, float4(center, 1.0)).xyz;
    viewPos.z = -viewPos.z;

    bool visible = sphereInsideFrustum(viewPos, radius, sf.Frustum, sf.ZNear, sf.ZFar);

#ifndef LATE
    // EARLY: write visibility for next frame so the Early/LATE decision stays stable.
    if (visible)
        VisBitSet(sf.ObjectVisBufferWrite, meshDrawIndex);
#endif

#ifdef LATE
    // LATE only needs to process objects that were NOT visible last frame.
    // (EARLY already handled those, and we must not count them twice.)
    if (VisBitTest(sf.ObjectVisBufferRead, meshDrawIndex))
        return;

    // Hi-Z: only the LATE variant probes the pyramid. EARLY trusts last frame's
    // visibility[] verdict and skips occlusion testing entirely (avoids stale-
    // pyramid feedback loops -- niagara two-phase semantics).
    if (visible)
    {
        float4 aabb;
        if (projectSphere(viewPos, radius, sf.ZNear, sf.P00, sf.P11, aabb))
        {
            float w     = aabb.z - aabb.x;
            float h     = aabb.w - aabb.y;
            uint2 p0    = BindlessTextureSize(pcCull.PyramidSlot);
            float level = ceil(log2(max(w * float(p0.x), h * float(p0.y))));
            float depth = SampleBindlessMaxLod(pcCull.PyramidSlot,
                                               (aabb.xy + aabb.zw) * 0.5,
                                               level).r;

            // Sphere closest-point NDC depth (perspectiveRH_ZO finite-far):
            float closestZ    = viewPos.z - radius;
            float depthSphere = sf.ZFar * (closestZ - sf.ZNear)
                              / (closestZ * (sf.ZFar - sf.ZNear));
            visible           = depthSphere <= depth;
        }
    }

    // LATE writes the canonical visibility for next frame's EARLY gating.
    if (visible)
        VisBitSet(sf.ObjectVisBufferWrite, meshDrawIndex);
#endif

    if (visible)
    {
        uint compactIndex;
        InterlockedAdd(icountBuf[pcCull.BucketIndex], 1, compactIndex);
        uint commandOffset           = (pcCull.DrawBase + compactIndex) * 3;
        icBuf[commandOffset + 0]  = (mesh.MeshletCount + TASK_WGSIZE - 1) / TASK_WGSIZE;
        icBuf[commandOffset + 1]  = 1;
        icBuf[commandOffset + 2]  = 1;
        vdiBuf[pcCull.DrawBase + compactIndex] = meshDrawIndex;
    }
}
