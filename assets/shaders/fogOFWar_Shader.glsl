#type vertex
#version 450


layout(push_constant) uniform PC { 
    mat4 uVP; mat4 uInvVP; vec2 mapMin; vec2 mapSize; float time; uint flags; 
} pc;

layout(location = 0) out vec2 vUV;

void main()
{
    const vec2 corners[6] = vec2[](
        vec2(0,0), vec2(1,0), vec2(1,1),
        vec2(0,0), vec2(1,1), vec2(0,1)
    );
    vec2 uv = corners[gl_VertexIndex];
    vUV = uv;
    vec2 worldPos = pc.mapMin + uv * pc.mapSize;
    gl_Position = pc.uVP * vec4(worldPos, 0.0, 1.0);
}

#type fragment
#version 450
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;
layout(set = 0, binding = 0) uniform sampler2D uVisibilityMap;
layout(push_constant) uniform PC {
    mat4 uVP; mat4 uInvVP; vec2 mapMin; vec2 mapSize; float time; uint flags;
} pc;

void main()
{
    float vis = texture(uVisibilityMap, vUV).r;
    outColor = vec4(vis, vis, vis, 1.0);
}