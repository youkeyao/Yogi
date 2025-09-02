#version 460 core

// Uniform buffer
layout(std140, binding=0) uniform Constants
{
    mat4 g_WorldViewProj;
};

// 输入 (来自顶点缓冲)
layout(location = 0) in vec3 Pos; // Position
layout(location = 1) in vec4 Color; // Color

// 输出给片段着色器
layout(location = 0) out vec4 v_Color;

void main()
{
    gl_Position = g_WorldViewProj * vec4(Pos, 1.0);
    v_Color = Color;
}