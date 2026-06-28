// Vertex shader for the Yogi-RHI ImGui renderer.
// Mirrors the stock imgui_impl_vulkan vertex stage: it maps ImGui's screen-space
// vertices into clip space using a push-constant scale + translate, and unpacks
// the packed RGBA8 vertex color into a float4.

struct ImGuiPush
{
    float2 Scale;
    float2 Translate;
};

[[vk::push_constant]] ConstantBuffer<ImGuiPush> pc;

struct VSIn
{
    [[vk::location(0)]] float2 Pos   : POSITION;
    [[vk::location(1)]] float2 UV    : TEXCOORD0;
    [[vk::location(2)]] float4 Color : COLOR0;
};

struct VSOut
{
    float4 Position : SV_Position;
    float4 Color    : COLOR0;
    float2 UV       : TEXCOORD0;
};

[shader("vertex")]
VSOut vsMain(VSIn input)
{
    VSOut o;
    o.Position = float4(input.Pos * pc.Scale + pc.Translate, 0.0, 1.0);
    o.Color    = input.Color;
    o.UV       = input.UV;
    return o;
}
