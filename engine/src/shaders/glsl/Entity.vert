#version 460 core

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
} scene_data;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in int a_EntityID;

layout(location = 0) flat out int v_EntityID;

void main()
{
    v_EntityID = a_EntityID;
    gl_Position = scene_data.proj_view * vec4(a_Position, 1.0);
}