import Shared.ShaderData;
#include "Renderer/Passes/OutlinePush.h"

[[vk::push_constant]] ConstantBuffer<OutlinePush> pcOutline;

[[vk::binding(0, 0)]] SamplerState g_DepthSampler;
[[vk::binding(1, 0)]] Texture2D    g_Depth;

float LinearizeDepth(float d, float zNear, float zFar)
{
    return zNear * zFar / (zFar - d * (zFar - zNear));
}

[shader("fragment")]
float4 fsMain(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target
{
    SceneFrame sf = ((SceneFrame*)pcOutline.SceneFrameAddr)[0];

    uint2  dim;
    g_Depth.GetDimensions(dim.x, dim.y);
    float2 texel = pcOutline.Thickness / float2(dim);

    float zNear = sf.ZNear;
    float zFar  = sf.ZFar;

    float c  = LinearizeDepth(g_Depth.SampleLevel(g_DepthSampler, uv, 0).r, zNear, zFar);
    float l  = LinearizeDepth(g_Depth.SampleLevel(g_DepthSampler, uv + float2(-texel.x, 0), 0).r, zNear, zFar);
    float r  = LinearizeDepth(g_Depth.SampleLevel(g_DepthSampler, uv + float2( texel.x, 0), 0).r, zNear, zFar);
    float t  = LinearizeDepth(g_Depth.SampleLevel(g_DepthSampler, uv + float2(0, -texel.y), 0).r, zNear, zFar);
    float b  = LinearizeDepth(g_Depth.SampleLevel(g_DepthSampler, uv + float2(0,  texel.y), 0).r, zNear, zFar);

    float delta = max(max(abs(c - l), abs(c - r)), max(abs(c - t), abs(c - b)));
    float edge  = step(pcOutline.DepthThreshold * c, delta);

    return float4(pcOutline.Color[0], pcOutline.Color[1], pcOutline.Color[2], edge);
}
