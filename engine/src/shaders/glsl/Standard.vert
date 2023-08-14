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
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec4 a_Color;
layout(location = 4) in float a_Metallic;
layout(location = 5) in float a_Roughness;
layout(location = 6) in int TEX_Albedo;

layout(location = 0) out vec3 v_Position;
layout(location = 1) out vec3 v_Normal;
layout(location = 2) out vec2 v_TexCoord;
layout(location = 3) out vec4 v_Color;
layout(location = 4) out float v_Metallic;
layout(location = 5) out float v_Roughness;
layout(location = 6) flat out int v_TexAlbedo;
layout(location = 7) out vec4 v_PosLightSpace;

void main()
{
    v_Position = a_Position;
    v_Normal = a_Normal;
    v_TexCoord = a_TexCoord;
    v_Color = a_Color;
    v_Metallic = a_Metallic;
    v_Roughness = a_Roughness;
    v_TexAlbedo = TEX_Albedo;

    v_PosLightSpace = scene_data.light_space_matrix * vec4(a_Position, 1.0);
    gl_Position = scene_data.proj_view * vec4(a_Position, 1.0);
}