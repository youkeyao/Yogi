#version 460 core

#extension GL_GOOGLE_include_directive: require
#extension GL_EXT_nonuniform_qualifier: require
#extension GL_EXT_shader_explicit_arithmetic_types: require
#extension GL_EXT_shader_16bit_storage: require
#extension GL_EXT_shader_8bit_storage: require

#include "Renderer/ShaderData.h"
#include "BufferRefs.glsl"

layout(binding = 0) uniform sampler   u_sampler;
layout(binding = 2) uniform texture2D u_textures[MAX_TEXTURES];

layout(push_constant) uniform ScenePushBlock
{
    ScenePush pcScene;
};

layout(location = 0) in      vec3 v_NormalShade;
layout(location = 1) in      vec2 v_UV;
layout(location = 2) in flat uint v_MaterialIndex;

layout(location = 0) out vec4 FragColor;

void main()
{
    SceneFrame        sf     = SceneFrameRef(pcScene.SceneFrameAddr).fd;
    MaterialBufferRef matBuf = MaterialBufferRef(sf.MaterialBuffer);
    MaterialData      mat    = matBuf.m[v_MaterialIndex];

    vec4 albedo = texture(sampler2D(u_textures[nonuniformEXT(mat.AlbedoTexture)], u_sampler), v_UV);

    vec3 Color = v_NormalShade * mat.BaseColor.rgb * albedo.rgb;

    #ifdef CONVERT_PS_OUTPUT_TO_GAMMA
        // 简单 gamma 矫正 (近似 1/2.2)
        Color = pow(Color, vec3(1.0 / 2.2));
    #endif

    FragColor = vec4(Color, 1.0);
}