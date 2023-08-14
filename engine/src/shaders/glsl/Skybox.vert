#version 460 core

struct SpotLight {
    vec4 color;
    vec3 pos;
    float cutoff;
};
struct PointLight {
    vec3 pos;
    float attenuation_parm;
    vec4 color;
};

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
    mat4 light_space_matrix;
    vec3 view_pos;
    int direction_light_num;
    vec4 directional_light_color;
    vec3 directional_light_direction;
    int spot_light_num;
    SpotLight spot_lights[4];
    PointLight point_lights[4];
    int point_light_num;
} scene_data;

layout(location = 0) in vec3 a_Position;

layout(location = 0) out vec3 v_TexCoord;

void main()
{
    v_TexCoord = a_Position - scene_data.view_pos;
    
    gl_Position = scene_data.proj_view * vec4(a_Position, 1.0);
    gl_Position.z = gl_Position.w - 0.00001;

    // ortho
    if (scene_data.proj_view[3][3] == 1) {
        float mag = min(abs(gl_Position.x), abs(gl_Position.y));
        gl_Position.x /= mag;
        gl_Position.y /= mag;
    }
}