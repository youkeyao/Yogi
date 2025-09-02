#version 460 core

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
} scene_data;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in int TEX_ID;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) flat out int v_TexID;

void main()
{
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TexID = TEX_ID;
    gl_Position = scene_data.proj_view * vec4(a_Position, 1.0);
}