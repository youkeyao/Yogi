#version 460 core

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
} scene_data;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;
layout(location = 2) in int TEX_ID;

layout(location = 0) out vec2 v_TexCoord;
layout(location = 1) flat out int v_TexID;

void main()
{
    v_TexCoord = a_TexCoord;
    v_TexID = TEX_ID;
    gl_Position = vec4(a_Position, 1.0);
}