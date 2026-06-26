import Shared.ShaderData;
#include "Renderer/Passes/OutlinePush.h"

[[vk::push_constant]] ConstantBuffer<OutlinePush> pcOutline;

// Dedicated binding set (created by OutlinePass): a clamp sampler + the
// scene depth texture. Depth (D24) reads back in .r as non-linear NDC depth.
[[vk::binding(0, 0)]] SamplerState g_DepthSampler;
[[vk::binding(1, 0)]] Texture2D    g_Depth;

// NDC depth [0,1] -> linear view-space depth, so the edge threshold is in
// world units and behaves consistently across the depth range.
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

    // Largest neighbour delta. Scale the threshold by depth so distant edges
    // (which have larger absolute depth gaps) aren't over-detected.
    float delta = max(max(abs(c - l), abs(c - r)), max(abs(c - t), abs(c - b)));
    float edge  = step(pcOutline.DepthThreshold * c, delta);

    // Solid-color overlay: alpha 1 on edges, 0 elsewhere. Pipeline alpha
    // blend (srcAlpha / 1-srcAlpha) composites this over the scene color.
    return float4(pcOutline.Color[0], pcOutline.Color[1], pcOutline.Color[2], edge);
}
