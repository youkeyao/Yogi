import Shared.ShaderData;
import Shared.Utils;
import Shared.Bindless;
import Materials.IMaterial;
import Materials.MaterialAttributes;

struct UnlitMaterial : IMaterial
{
    [Color(1, 1, 1, 1)] float4 Tint;
    [Texture]           uint   AlbedoTexture;

    float4 shade(ShadingContext ctx)
    {
        float4 albedo = SampleBindless(this.AlbedoTexture, ctx.uv0);
        return float4(this.Tint.rgb * albedo.rgb, this.Tint.a * albedo.a);
    }
};
