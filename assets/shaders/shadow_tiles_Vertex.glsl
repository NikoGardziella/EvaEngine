#type vertex
#version 460

struct Instance {
    vec2  worldPos;
    vec2  size;
    float rotation;
    float zSortKey;
    uint  slot;
    uint  flags;
    uint  _pad0;
    uint  _pad1;
    uvec2 uvMin16;
    uvec2 uvMax16;
};

layout(push_constant) uniform PC { 
    mat4 lightSpaceMatrix; 
} pc;

layout(std430, set=0, binding=2) readonly buffer Instances {
    Instance inst[];
};

const vec2 quad[4] = vec2[](
    vec2(0.0,0.0), vec2(1.0,0.0),
    vec2(0.0,1.0), vec2(1.0,1.0)
);

void main()
{
    uint i = gl_InstanceIndex;
    vec2 q = quad[gl_VertexIndex];
    vec2 center = inst[i].worldPos + vec2(0.0, 0.5 * inst[i].size.y);
    vec2 local = (q - vec2(0.5)) * inst[i].size;
    float ang = inst[i].rotation;
    float c   = cos(ang);
    float s   = sin(ang);
    mat2 R    = mat2(c, s, -s, c);
    vec2 rotated = R * local;
    vec2 pos = center + rotated;
    
    gl_Position = pc.lightSpaceMatrix * vec4(pos, 0.0, 1.0);
}