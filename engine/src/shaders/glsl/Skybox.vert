#version 460 core

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
    vec3 view_pos;
} scene_data;

layout(location = 0) in vec3 a_Position;
layout(location = 1) in int TEX_Skybox;

layout(location = 0) out vec3 v_TexCoord;
layout(location = 1) flat out int v_TexSkybox;

void main()
{
    v_TexCoord = a_Position;
    v_TexSkybox = TEX_Skybox;
    
    gl_Position = scene_data.proj_view * vec4(a_Position + scene_data.view_pos, 1.0);
    gl_Position.z = gl_Position.w - 0.00001;

    // ortho
    if (scene_data.proj_view[3][3] == 1) {
        float mag = min(abs(gl_Position.x), abs(gl_Position.y));
        gl_Position.x /= mag;
        gl_Position.y /= mag;
    }
}