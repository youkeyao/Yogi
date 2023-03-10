#version 460 core

layout(set = 0, binding = 0) uniform SceneData
{
    mat4 u_projection_view;
};

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in int a_TexID;
layout(location = 4) in int a_EntityID;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_TexCoord;
layout(location = 2) flat out int v_TexID;
layout(location = 3) flat out int v_EntityID;

void main()
{
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TexID = a_TexID;
    v_EntityID = a_EntityID;
    gl_Position = u_projection_view * vec4(a_Position, 1.0);
}