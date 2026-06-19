import Shared.ShaderData;
import Shared.Utils;
import Shared.Bindless;
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

    ShadingContext ctx;
    ctx.worldPos    = float3(0, 0, 0);
    ctx.worldNormal = i.NormalShade;
    ctx.viewDir     = float3(0, 0, 1);
    ctx.uv0         = i.UV;

    float4 color = m.shade(ctx);

#ifdef CONVERT_PS_OUTPUT_TO_GAMMA
    color.rgb = pow(color.rgb, float3(1.0 / 2.2, 1.0 / 2.2, 1.0 / 2.2));
#endif

    return color;
}
