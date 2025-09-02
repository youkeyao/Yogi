#version 460 core

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
    vec3 view_pos;
} scene_data;

layout(location = 0) out vec4 color;

layout(location = 0) in vec3 v_TexCoord;
layout(location = 1) flat in int v_TexSkybox;

layout(binding = 1) uniform sampler2D u_Textures[32];

#define PI 3.14159265359

vec2 sampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv /= vec2(2.0 * PI, PI);
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

void main()
{
    vec2 texcoord = sampleSphericalMap(normalize(-v_TexCoord));
    color = vec4(v_TexCoord, 1);

    switch (v_TexSkybox) {
        case  0: color = texture(u_Textures[ 0], texcoord); break;
        case  1: color = texture(u_Textures[ 1], texcoord); break;
        case  2: color = texture(u_Textures[ 2], texcoord); break;
        case  3: color = texture(u_Textures[ 3], texcoord); break;
        case  4: color = texture(u_Textures[ 4], texcoord); break;
        case  5: color = texture(u_Textures[ 5], texcoord); break;
        case  6: color = texture(u_Textures[ 6], texcoord); break;
        case  7: color = texture(u_Textures[ 7], texcoord); break;
        case  8: color = texture(u_Textures[ 8], texcoord); break;
        case  9: color = texture(u_Textures[ 9], texcoord); break;
        case 10: color = texture(u_Textures[10], texcoord); break;
        case 11: color = texture(u_Textures[11], texcoord); break;
        case 12: color = texture(u_Textures[12], texcoord); break;
        case 13: color = texture(u_Textures[13], texcoord); break;
        case 14: color = texture(u_Textures[14], texcoord); break;
        case 15: color = texture(u_Textures[15], texcoord); break;
        case 16: color = texture(u_Textures[16], texcoord); break;
        case 17: color = texture(u_Textures[17], texcoord); break;
        case 18: color = texture(u_Textures[18], texcoord); break;
        case 19: color = texture(u_Textures[19], texcoord); break;
        case 20: color = texture(u_Textures[20], texcoord); break;
        case 21: color = texture(u_Textures[21], texcoord); break;
        case 22: color = texture(u_Textures[22], texcoord); break;
        case 23: color = texture(u_Textures[23], texcoord); break;
        case 24: color = texture(u_Textures[24], texcoord); break;
        case 25: color = texture(u_Textures[25], texcoord); break;
        case 26: color = texture(u_Textures[26], texcoord); break;
        case 27: color = texture(u_Textures[27], texcoord); break;
        case 28: color = texture(u_Textures[28], texcoord); break;
        case 29: color = texture(u_Textures[29], texcoord); break;
        case 30: color = texture(u_Textures[30], texcoord); break;
        case 31: color = texture(u_Textures[31], texcoord); break;
    }
}