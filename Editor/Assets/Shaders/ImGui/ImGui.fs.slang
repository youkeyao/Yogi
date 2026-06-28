[[vk::binding(0, 0)]] SamplerState g_Sampler;
[[vk::binding(1, 0)]] Texture2D    g_Texture;

[shader("fragment")]
float4 fsMain(float4 color : COLOR0, float2 uv : TEXCOORD0) : SV_Target
{
    return color * g_Texture.Sample(g_Sampler, uv);
}
