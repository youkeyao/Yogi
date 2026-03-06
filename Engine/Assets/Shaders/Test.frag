#version 460 core

layout(location = 0) in vec4 v_Color;
layout(location = 0) out vec4 FragColor;

void main()
{
    vec4 Color = v_Color;

    #ifdef CONVERT_PS_OUTPUT_TO_GAMMA
        // 简单 gamma 矫正 (近似 1/2.2)
        Color.rgb = pow(Color.rgb, vec3(1.0 / 2.2));
    #endif

    FragColor = Color;
}