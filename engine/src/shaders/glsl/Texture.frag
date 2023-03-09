#version 460 core
layout(location = 0) out vec4 color;
layout(location = 1) out int entity_id;

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_TexCoord;
layout(location = 2) flat in int v_TexID;
layout(location = 3) flat in int v_EntityID;

layout(set = 0, binding = 0) uniform sampler2D u_Textures[32];

void main()
{
    color = v_Color;
    switch (v_TexID) {
        case  1: color *= texture(u_Textures[ 1], v_TexCoord); break;
        case  2: color *= texture(u_Textures[ 2], v_TexCoord); break;
        case  3: color *= texture(u_Textures[ 3], v_TexCoord); break;
        case  4: color *= texture(u_Textures[ 4], v_TexCoord); break;
        case  5: color *= texture(u_Textures[ 5], v_TexCoord); break;
        case  6: color *= texture(u_Textures[ 6], v_TexCoord); break;
        case  7: color *= texture(u_Textures[ 7], v_TexCoord); break;
        case  8: color *= texture(u_Textures[ 8], v_TexCoord); break;
        case  9: color *= texture(u_Textures[ 9], v_TexCoord); break;
        case 10: color *= texture(u_Textures[10], v_TexCoord); break;
        case 11: color *= texture(u_Textures[11], v_TexCoord); break;
        case 12: color *= texture(u_Textures[12], v_TexCoord); break;
        case 13: color *= texture(u_Textures[13], v_TexCoord); break;
        case 14: color *= texture(u_Textures[14], v_TexCoord); break;
        case 15: color *= texture(u_Textures[15], v_TexCoord); break;
        case 16: color *= texture(u_Textures[16], v_TexCoord); break;
        case 17: color *= texture(u_Textures[17], v_TexCoord); break;
        case 18: color *= texture(u_Textures[18], v_TexCoord); break;
        case 19: color *= texture(u_Textures[19], v_TexCoord); break;
        case 20: color *= texture(u_Textures[20], v_TexCoord); break;
        case 21: color *= texture(u_Textures[21], v_TexCoord); break;
        case 22: color *= texture(u_Textures[22], v_TexCoord); break;
        case 23: color *= texture(u_Textures[23], v_TexCoord); break;
        case 24: color *= texture(u_Textures[24], v_TexCoord); break;
        case 25: color *= texture(u_Textures[25], v_TexCoord); break;
        case 26: color *= texture(u_Textures[26], v_TexCoord); break;
        case 27: color *= texture(u_Textures[27], v_TexCoord); break;
        case 28: color *= texture(u_Textures[28], v_TexCoord); break;
        case 29: color *= texture(u_Textures[29], v_TexCoord); break;
        case 30: color *= texture(u_Textures[30], v_TexCoord); break;
        case 31: color *= texture(u_Textures[31], v_TexCoord); break;
    }
    entity_id = v_EntityID;
}