import Shared.ShaderData;
import Materials.IMaterial;
import Materials.Standard;
import Materials.Unlit;

[[vk::push_constant]] ConstantBuffer<ScenePush> pcScene;

[shader("fragment")]
float4 fsMain<M : IMaterial>(MeshletVertex i) : SV_Target
{
    SceneFrame sf = ((SceneFrame*)pcScene.SceneFrameAddr)[0];
    M* matBuf = (M*)pcScene.MaterialBufferAddr;
    M m = matBuf[i.MaterialIndex];

    // Force-read all vertex inputs so they are not optimized away.
    float2 uv = i.UV;
    float3 ns = i.NormalShade;
    int matIndex = i.MaterialIndex;

    ShadingContext ctx;
    ctx.worldPos    = float3(0, 0, 0);
    ctx.worldNormal = i.NormalShade;
    ctx.viewDir     = float3(0, 0, 1);
    ctx.uv0         = i.UV;

    float4 color = m.shade(ctx);

    return color;
}
