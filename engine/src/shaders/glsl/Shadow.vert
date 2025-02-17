#version 460 core

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
} scene_data;

layout(location = 0) in vec3 a_Position;

layout(location = 0) out vec4 v_Pos;

void main()
{
    v_Pos = scene_data.proj_view * vec4(a_Position, 1.0);
    gl_Position = v_Pos;
}