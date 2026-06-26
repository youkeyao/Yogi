// Fullscreen-triangle vertex shader for the screen-space outline pass.
// Emits a single oversized triangle (3 vertices, no vertex buffer) that
// covers the whole viewport; the fragment shader does depth-edge detection.

struct VSOut
{
    float4 Position : SV_Position;
    float2 UV       : TEXCOORD0;
};

[shader("vertex")]
VSOut vsMain(uint vid : SV_VertexID)
{
    VSOut o;
    // (0,0) (2,0) (0,2) in UV -> clip covers [-1,1] with one triangle.
    o.UV       = float2((vid << 1) & 2, vid & 2);
    o.Position = float4(o.UV * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    return o;
}
