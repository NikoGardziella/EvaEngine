#type vertex
#version 460
layout(location = 0) in vec3 a_Position;

layout(push_constant) uniform PC { mat4 lightVP; } pc;

void main()
{
    gl_Position = pc.lightVP * vec4(a_Position, 1.0);
}
