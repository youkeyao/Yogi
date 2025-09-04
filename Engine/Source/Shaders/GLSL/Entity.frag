#version 460 core

layout(location = 0) out int id;

layout(location = 0) flat in int v_EntityID;

layout(binding = 1) uniform sampler2D u_Textures[32];

void main()
{
    id = v_EntityID;
}