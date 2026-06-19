import Shared.ShaderData;
import Shared.Utils;
import Shared.Bindless;

struct ShadingContext
{
    float3 worldPos;
    float3 worldNormal;
    float3 viewDir;
    float2 uv0;
};

interface IMaterial
{
    float3 vertexHook(float3 worldPos, ShadingContext ctx) { return worldPos; }

    float4 shade(ShadingContext ctx);
};
