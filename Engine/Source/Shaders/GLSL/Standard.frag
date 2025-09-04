#version 460 core

#define PI 3.14159265359

struct SpotLight {
    vec4 color;
    vec3 pos;
    float inner_cutoff;
    vec3 direction;
    float outer_cutoff;
    mat4 spot_light_space_matrix;
};
struct PointLight {
    vec3 pos;
    float attenuation_parm;
    vec4 color;
    mat4 point_light_space_matrix;
};

layout(binding = 0) uniform SceneData {
    mat4 proj_view;
    vec3 view_pos;
    int spot_light_num;
    vec4 directional_light_color;
    vec3 directional_light_direction;
    int point_light_num;
    mat4 directional_light_space_matrix;
    SpotLight spot_lights[4];
    PointLight point_lights[4];
} scene_data;

layout(location = 0) out vec4 color;

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TexCoord;
layout(location = 3) in vec4 v_Color;
layout(location = 4) in float v_Metallic;
layout(location = 5) in float v_Roughness;
layout(location = 6) flat in int v_TexAlbedo;

layout(binding = 1) uniform sampler2D u_Textures[32];

// --------------------------------------------------------------------------

vec4 id_sample_texture(int id, vec2 uv)
{
    vec4 color = vec4(1, 1, 1, 1);
    switch (id) {
        case  0: color = texture(u_Textures[ 0], uv); break;
        case  1: color = texture(u_Textures[ 1], uv); break;
        case  2: color = texture(u_Textures[ 2], uv); break;
        case  3: color = texture(u_Textures[ 3], uv); break;
        case  4: color = texture(u_Textures[ 4], uv); break;
        case  5: color = texture(u_Textures[ 5], uv); break;
        case  6: color = texture(u_Textures[ 6], uv); break;
        case  7: color = texture(u_Textures[ 7], uv); break;
        case  8: color = texture(u_Textures[ 8], uv); break;
        case  9: color = texture(u_Textures[ 9], uv); break;
        case 10: color = texture(u_Textures[10], uv); break;
        case 11: color = texture(u_Textures[11], uv); break;
        case 12: color = texture(u_Textures[12], uv); break;
        case 13: color = texture(u_Textures[13], uv); break;
        case 14: color = texture(u_Textures[14], uv); break;
        case 15: color = texture(u_Textures[15], uv); break;
        case 16: color = texture(u_Textures[16], uv); break;
        case 17: color = texture(u_Textures[17], uv); break;
        case 18: color = texture(u_Textures[18], uv); break;
        case 19: color = texture(u_Textures[19], uv); break;
        case 20: color = texture(u_Textures[20], uv); break;
        case 21: color = texture(u_Textures[21], uv); break;
        case 22: color = texture(u_Textures[22], uv); break;
        case 23: color = texture(u_Textures[23], uv); break;
        case 24: color = texture(u_Textures[24], uv); break;
        case 25: color = texture(u_Textures[25], uv); break;
        case 26: color = texture(u_Textures[26], uv); break;
        case 27: color = texture(u_Textures[27], uv); break;
        case 28: color = texture(u_Textures[28], uv); break;
        case 29: color = texture(u_Textures[29], uv); break;
        case 30: color = texture(u_Textures[30], uv); break;
        case 31: color = texture(u_Textures[31], uv); break;
    }
    return color;
}

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

float ShadowCalculation(vec4 fragPosLightSpace, int shadowMapId)
{
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    float closestDepth = id_sample_texture(shadowMapId, projCoords.xy).r;
    float currentDepth = projCoords.z;
    vec3 normal = normalize(v_Normal);
    vec3 lightDir = -normalize(scene_data.directional_light_direction);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.005);

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_Textures[0], 0);
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            float pcfDepth = id_sample_texture(shadowMapId, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if (projCoords.z > 1.0) shadow = 0.0;

    return shadow;
}

// --------------------------------------------------------------------------------------

void main()
{
    vec4 albedo = v_Color * id_sample_texture(v_TexAlbedo, v_TexCoord);

    vec3 N = normalize(v_Normal);
    vec3 V = normalize(scene_data.view_pos - v_Position);

    vec3 Lo = vec3(0.0);

    int shadowMapId = 0;
    if (length(scene_data.directional_light_color) > 0) {
        vec3 L = -normalize(scene_data.directional_light_direction);
        vec3 radiance = scene_data.directional_light_color.xyz;
        vec3 pbr_color = PBR(L, N, V, albedo.xyz, radiance, v_Metallic, v_Roughness);

        vec4 posLightSpace = scene_data.directional_light_space_matrix * vec4(v_Position, 1.0);
        Lo += pbr_color * (1 - ShadowCalculation(posLightSpace, shadowMapId));
        shadowMapId += 1;
    }
    for (int i = 0; i < scene_data.spot_light_num; i ++) {
        vec3 L = normalize(scene_data.spot_lights[i].pos - v_Position);
        vec3 radiance = scene_data.spot_lights[i].color.xyz;
        vec3 pbr_color = PBR(L, N, V, albedo.xyz, radiance, v_Metallic, v_Roughness);

        float theta = dot(L, normalize(-scene_data.spot_lights[i].direction)); 
        float epsilon = scene_data.spot_lights[i].inner_cutoff - scene_data.spot_lights[i].outer_cutoff;
        float intensity = clamp((theta - scene_data.spot_lights[i].outer_cutoff) / epsilon, 0.0, 1.0);

        vec4 posLightSpace = scene_data.spot_lights[i].spot_light_space_matrix * vec4(v_Position, 1.0);
        Lo += pbr_color * (1 - ShadowCalculation(posLightSpace, shadowMapId)) * intensity;
        shadowMapId += 1;
    }
    for (int i = 0; i < scene_data.point_light_num; i ++) {
        vec3 L = normalize(scene_data.point_lights[i].pos - v_Position);
        float distance = length(scene_data.point_lights[i].pos - v_Position);
        float attenuation = 1.0 / (scene_data.point_lights[i].attenuation_parm * (distance * distance));
        vec3 radiance = scene_data.point_lights[i].color.xyz * attenuation;
        vec3 pbr_color = PBR(L, N, V, albedo.xyz, radiance, v_Metallic, v_Roughness);

        int id = shadowMapId;
        float mag=max(max(abs(L.x), abs(L.y)), abs(L.z));
        vec4 posLightSpace = vec4(0, 0, 0, 1);
        if (mag == abs(L.x)) {
            if (L.x > 0) {
                id += 1;
                vec3 old_dir = v_Position - scene_data.point_lights[i].pos;
                vec3 dir = old_dir;
                dir.x = -old_dir.x;
                dir.z = -old_dir.z;
                posLightSpace = scene_data.point_lights[i].point_light_space_matrix * vec4(scene_data.point_lights[i].pos + dir, 1.0);
            }
            else if (L.x < 0) {
                id += 0;
                posLightSpace = scene_data.point_lights[i].point_light_space_matrix * vec4(v_Position, 1.0);
            }
        }
        else if (mag == abs(L.y)) {
            if (L.y > 0) {
                id += 3;
                vec3 old_dir = v_Position - scene_data.point_lights[i].pos;
                vec3 dir = old_dir;
                dir.x = -old_dir.y;
                dir.y = old_dir.z;
                dir.z = -old_dir.x;
                posLightSpace = scene_data.point_lights[i].point_light_space_matrix * vec4(scene_data.point_lights[i].pos + dir, 1.0);
            }
            else if (L.y < 0) {
                id += 2;
                vec3 old_dir = v_Position - scene_data.point_lights[i].pos;
                vec3 dir = old_dir;
                dir.x = old_dir.y;
                dir.y = -old_dir.z;
                dir.z = -old_dir.x;
                posLightSpace = scene_data.point_lights[i].point_light_space_matrix * vec4(scene_data.point_lights[i].pos + dir, 1.0);
            }
        }
        else if (mag == abs(L.z)) {
            if (L.z > 0) {
                id += 5;
                vec3 old_dir = v_Position - scene_data.point_lights[i].pos;
                vec3 dir = old_dir;
                dir.x = -old_dir.z;
                dir.z = old_dir.x;
                posLightSpace = scene_data.point_lights[i].point_light_space_matrix * vec4(scene_data.point_lights[i].pos + dir, 1.0);
            }
            else if (L.z < 0) {
                id += 4;
                vec3 old_dir = v_Position - scene_data.point_lights[i].pos;
                vec3 dir = old_dir;
                dir.x = old_dir.z;
                dir.z = -old_dir.x;
                posLightSpace = scene_data.point_lights[i].point_light_space_matrix * vec4(scene_data.point_lights[i].pos + dir, 1.0);
            }
        }

        Lo += pbr_color * (1 - ShadowCalculation(posLightSpace, id));
        shadowMapId += 6;
    }

    Lo += vec3(0.03) * albedo.xyz;

    Lo = Lo / (Lo + vec3(1.0));
    Lo = pow(Lo, vec3(1.0/2.2));

    color = vec4(Lo, 1);
}