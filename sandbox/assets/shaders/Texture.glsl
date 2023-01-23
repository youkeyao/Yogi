#type vertex
#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec2 a_TexCoord;

uniform mat4 u_view_projection;
uniform mat4 u_transform;

out vec2 v_TexCoord;
void main()
{
    v_TexCoord = a_TexCoord;
    gl_Position = u_view_projection * u_transform * vec4(a_Position, 1.0);
}

#type fragment
#version 330 core
layout(location = 0) out vec4 color;

uniform vec4 u_color;
uniform sampler2D u_texture;
uniform float u_tiling_factor;

in vec2 v_TexCoord;
void main()
{
    color = texture(u_texture, v_TexCoord * u_tiling_factor) * u_color;
}