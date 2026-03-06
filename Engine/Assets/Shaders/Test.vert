#version 460 core

struct Vertex
{
    float vx, vy, vz;
    float nx, ny, nz;
    float tu, tv;
};

// Storage buffer
layout(binding=0) readonly buffer Vertices
{
    Vertex vertices[];
};

// 输出给片段着色器
layout(location = 0) out vec4 v_Color;

void main()
{
    Vertex v = vertices[gl_VertexIndex];
    vec3 position = vec3(v.vx, v.vy, v.vz);
    vec3 normal = vec3(v.nx, v.ny, v.nz);
    gl_Position = vec4(position, 1.0);
    v_Color = vec4(v.tu, v.tv, 0, 1);
}