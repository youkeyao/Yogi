#version 460 core

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
    mat4 light_space_matrix;
    vec3 view_pos;
} scene_data;

layout(location = 0) out vec4 color;

layout(location = 0) in vec3 v_TexCoord;
layout(location = 1) flat in int v_TexSkybox;

layout(binding = 1) uniform sampler2D u_Textures[32];

void main()
{
    vec2 texcoord = vec2(0, 0);
    color = vec4(v_TexCoord, 1);

    float mag=max(max(abs(v_TexCoord.x), abs(v_TexCoord.y)), abs(v_TexCoord.z));
    if (mag == abs(v_TexCoord.x)) {
        if (v_TexCoord.x > 0) {
            texcoord = vec2((1-(v_TexCoord.z+1)/2)/6, (v_TexCoord.y+1)/2);
        }
        else if (v_TexCoord.x < 0) {
            texcoord = vec2((v_TexCoord.z+1)/2/6 + 1.0/6,(v_TexCoord.y+1)/2);
        }
    }
    else if (mag == abs(v_TexCoord.y)) {
        if (v_TexCoord.y > 0)
            texcoord = vec2((v_TexCoord.x+1)/2/6 + 2.0/6,1-(v_TexCoord.z+1)/2);
        else if (v_TexCoord.y < 0)
            texcoord = vec2((v_TexCoord.x+1)/2/6 + 3.0/6,(v_TexCoord.z+1)/2);
    }
    else if (mag == abs(v_TexCoord.z)) {
        if (v_TexCoord.z > 0)
            texcoord = vec2((v_TexCoord.x+1)/2/6 + 4.0/6,(v_TexCoord.y+1)/2);
        else if (v_TexCoord.z < 0)
            texcoord = vec2((1-(v_TexCoord.x+1)/2)/6 + 5.0/6,(v_TexCoord.y+1)/2);
    }

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