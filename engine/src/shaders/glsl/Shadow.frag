#version 460 core

layout(location = 0) out vec4 color;

layout(location = 0) in vec4 v_Pos;

layout(binding = 1) uniform sampler2D u_Textures[32];

void main()
{
    color = vec4(v_Pos.z / v_Pos.w, 0, 0, 1);
}