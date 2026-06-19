import Shared.ShaderData;
import Shared.Utils;
import Shared.Bindless;
import Materials.IMaterial;
import Materials.MaterialAttributes;

struct StandardMaterial : IMaterial
{
    [Color(1, 1, 1, 1)] float4 BaseColor;
    [Texture]           uint   AlbedoTexture;

    float4 shade(ShadingContext ctx)
    {
        // SampleBindless wraps `g_Textures[NUR(slot)].Sample(g_Sampler, uv)`.
        float4 albedo = SampleBindless(this.AlbedoTexture, ctx.uv0);

        // Match the previous Test.frag: NormalShade is normal*0.5 + 0.5,
        // i.e. baked into ctx.worldNormal here as the "fake N.L" in Phase 3.
        float3 color = ctx.worldNormal * this.BaseColor.rgb * albedo.rgb;

        return float4(color, 1.0);
    }
};
