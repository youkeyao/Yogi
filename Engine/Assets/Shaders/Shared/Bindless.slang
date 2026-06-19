import Shared.ShaderData;

[[vk::binding(0, 0)]] SamplerState g_Sampler;
[[vk::binding(1, 0)]] SamplerState g_SamplerMax;
[[vk::binding(2, 0)]] Texture2D g_Textures[MAX_TEXTURES];

float4 SampleBindless(uint slot, float2 uv)
{
    return g_Textures[NonUniformResourceIndex(slot)].Sample(g_Sampler, uv);
}

float4 SampleBindlessLevel(uint slot, float2 uv, float level)
{
    return g_Textures[NonUniformResourceIndex(slot)].SampleLevel(g_Sampler, uv, level);
}

float4 SampleBindlessMaxLod(uint slot, float2 uv, float level)
{
    return g_Textures[NonUniformResourceIndex(slot)].SampleLevel(g_SamplerMax, uv, level);
}

uint2 BindlessTextureSize(uint slot)
{
    uint2 dim;
    g_Textures[NonUniformResourceIndex(slot)].GetDimensions(dim.x, dim.y);
    return dim;
}
