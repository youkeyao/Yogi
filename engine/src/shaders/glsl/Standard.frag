#version 460 core

#define PI 3.14159265359

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

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;
layout(location = 3) in vec4 v_Color;
layout(location = 4) in float v_Metallic;
layout(location = 5) in float v_Roughness;
layout(location = 6) flat in int v_TexAlbedo;
layout(location = 7) in vec4 v_PosLightSpace;

layout(binding = 1) uniform sampler2D u_Textures[32];

// ----------------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a      = roughness*roughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return num / denom;
}
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 PBR(vec3 L, vec3 N, vec3 V, vec3 albedo, vec3 radiance, float metallic, float roughness)
{
    vec3 H = normalize(V + L);

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G   = GeometrySmith(N, V, L, roughness);
    vec3  F   = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3  nominator   = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.001;
    vec3  specular    = nominator / denominator;

    float NdotL = max(dot(N, L), 0.0);
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    float closestDepth = texture(u_Textures[0], projCoords.xy).r; 
    float currentDepth = projCoords.z;
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = -normalize(scene_data.directional_light_direction);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_Textures[0], 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = texture(u_Textures[0], projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (projCoords.z > 1.0) shadow = 0.0;
        
    return shadow;
}

void main()
{
    vec4 albedo = v_Color;
    switch (v_TexAlbedo) {
        case  0: albedo *= texture(u_Textures[ 0], v_TexCoord); break;
        case  1: albedo *= texture(u_Textures[ 1], v_TexCoord); break;
        case  2: albedo *= texture(u_Textures[ 2], v_TexCoord); break;
        case  3: albedo *= texture(u_Textures[ 3], v_TexCoord); break;
        case  4: albedo *= texture(u_Textures[ 4], v_TexCoord); break;
        case  5: albedo *= texture(u_Textures[ 5], v_TexCoord); break;
        case  6: albedo *= texture(u_Textures[ 6], v_TexCoord); break;
        case  7: albedo *= texture(u_Textures[ 7], v_TexCoord); break;
        case  8: albedo *= texture(u_Textures[ 8], v_TexCoord); break;
        case  9: albedo *= texture(u_Textures[ 9], v_TexCoord); break;
        case 10: albedo *= texture(u_Textures[10], v_TexCoord); break;
        case 11: albedo *= texture(u_Textures[11], v_TexCoord); break;
        case 12: albedo *= texture(u_Textures[12], v_TexCoord); break;
        case 13: albedo *= texture(u_Textures[13], v_TexCoord); break;
        case 14: albedo *= texture(u_Textures[14], v_TexCoord); break;
        case 15: albedo *= texture(u_Textures[15], v_TexCoord); break;
        case 16: albedo *= texture(u_Textures[16], v_TexCoord); break;
        case 17: albedo *= texture(u_Textures[17], v_TexCoord); break;
        case 18: albedo *= texture(u_Textures[18], v_TexCoord); break;
        case 19: albedo *= texture(u_Textures[19], v_TexCoord); break;
        case 20: albedo *= texture(u_Textures[20], v_TexCoord); break;
        case 21: albedo *= texture(u_Textures[21], v_TexCoord); break;
        case 22: albedo *= texture(u_Textures[22], v_TexCoord); break;
        case 23: albedo *= texture(u_Textures[23], v_TexCoord); break;
        case 24: albedo *= texture(u_Textures[24], v_TexCoord); break;
        case 25: albedo *= texture(u_Textures[25], v_TexCoord); break;
        case 26: albedo *= texture(u_Textures[26], v_TexCoord); break;
        case 27: albedo *= texture(u_Textures[27], v_TexCoord); break;
        case 28: albedo *= texture(u_Textures[28], v_TexCoord); break;
        case 29: albedo *= texture(u_Textures[29], v_TexCoord); break;
        case 30: albedo *= texture(u_Textures[30], v_TexCoord); break;
        case 31: albedo *= texture(u_Textures[31], v_TexCoord); break;
    }

    vec3 N = normalize(v_Normal);
    vec3 V = normalize(scene_data.view_pos - v_Position);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < scene_data.direction_light_num; i ++) {
        vec3 L = -normalize(scene_data.directional_light_direction);
        vec3 radiance = scene_data.directional_light_color.xyz;
        Lo += PBR(L, N, V, albedo.xyz, radiance, v_Metallic, v_Roughness);
    }
    for (int i = 0; i < scene_data.point_light_num; i ++) {
        vec3 L = normalize(scene_data.point_lights[i].pos - v_Position);
        float distance = length(scene_data.point_lights[i].pos - v_Position);
        float attenuation = 1.0 / (scene_data.point_lights[i].attenuation_parm * (distance * distance));
        vec3 radiance = scene_data.point_lights[i].color.xyz * attenuation;
        Lo += PBR(L, N, V, albedo.xyz, radiance, v_Metallic, v_Roughness);
    }

    // shadow
    Lo *= 1 - ShadowCalculation(v_PosLightSpace);

    Lo += vec3(0.03) * albedo.xyz;

    Lo = Lo / (Lo + vec3(1.0));
    Lo = pow(Lo, vec3(1.0/2.2));

    color = vec4(Lo, 1);
}