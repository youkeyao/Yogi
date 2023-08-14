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

layout(location = 0) out vec4 color;

layout(location = 0) in vec3 v_TexCoord;

layout(binding = 1) uniform sampler2D u_Textures[32];

vec4 texture_cube(vec3 tex)
{
    float mag=max(max(abs(tex.x),abs(tex.y)),abs(tex.z));
    if (mag == abs(tex.x))
    {
        if (tex.x > 0) {
            return texture(u_Textures[0], vec2((1-(tex.z+1)/2)/6,(tex.y+1)/2));
        }
        else if (tex.x < 0) {
            return texture(u_Textures[0], vec2((tex.z+1)/2/6 + 1.0/6,(tex.y+1)/2));
        }
    }
    else if (mag == abs(tex.y))
    {
        if (tex.y > 0)
            return texture(u_Textures[0], vec2((tex.x+1)/2/6 + 2.0/6,1-(tex.z+1)/2));
        else if (tex.y < 0)
            return texture(u_Textures[0], vec2((tex.x+1)/2/6 + 3.0/6,(tex.z+1)/2));
    }
    else if (mag == abs(tex.z))
    {
        if (tex.z > 0)
            return texture(u_Textures[0], vec2((tex.x+1)/2/6 + 4.0/6,(tex.y+1)/2));
        else if (tex.z < 0)
            return texture(u_Textures[0], vec2((1-(tex.x+1)/2)/6 + 5.0/6,(tex.y+1)/2));
    }
}

void main()
{
    color = texture_cube(v_TexCoord);
}