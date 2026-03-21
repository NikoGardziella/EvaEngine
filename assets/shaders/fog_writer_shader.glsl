#type vertex
#version 450
layout(location = 0) in vec2 inPos; // World XZ

layout(push_constant) uniform PC { 
    mat4 uVP;
    mat4 uInvVP;
    vec2 mapMin; 
    vec2 mapSize;
    float time;
    uint flags; 
} pc;

void main()
{
  
    vec2 uv = (inPos - pc.mapMin) / pc.mapSize;
    gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
}

#type fragment
#version 450
layout(location = 0) out float outVisibility;
void main()
{
    outVisibility = 1.0;
}