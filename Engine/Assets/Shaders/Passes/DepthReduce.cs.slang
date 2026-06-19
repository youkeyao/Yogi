import Shared.ShaderData;

struct DepthReducePush
{
    uint2 imageSize;
};
[[vk::push_constant]] ConstantBuffer<DepthReducePush> u_push;

[[vk::binding(0, 0)]] SamplerState u_samplerMax;
[[vk::binding(1, 0)]] Texture2D u_srcMip;
[[vk::binding(2, 0)]][[format("r32f")]] RWTexture2D<float> u_dstMip;

[shader("compute")]
[numthreads(DEPTH_REDUCE_WGSIZE, DEPTH_REDUCE_WGSIZE, 1)]
void csMain(uint3 dtid : SV_DispatchThreadID)
{
    uint2 pos = dtid.xy;
    if (pos.x >= u_push.imageSize.x || pos.y >= u_push.imageSize.y)
        return;

    float2 uv    = (float2(pos) + float2(0.5, 0.5)) / float2(u_push.imageSize);
    float  depth = u_srcMip.SampleLevel(u_samplerMax, uv, 0).r;

    u_dstMip[int2(pos)] = depth;
}
