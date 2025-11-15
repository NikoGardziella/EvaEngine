#type vertex
#version 460
layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNrm;
layout(location=2) in vec2 inUV;

layout(location=0) out vec3 vNrmW;
layout(location=1) out vec2 vUV;
layout(location=2) out vec3 vPosW;

layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialId;
    uint submeshId;
    uint flags;
} pc;

layout(set=0, binding=0) uniform CameraUBO {
    mat4 uView;
    mat4 uProj;
} cam;

layout(std430, set=0, binding=1) readonly buffer InstanceData {
    mat4 world[];
} gInstances;

void main() {
    mat4 W  = gInstances.world[pc.instanceIndex];
    mat4 VP = cam.uProj * cam.uView;

    vec4 Pw = W * vec4(inPos, 1.0);
    gl_Position = VP * Pw;

   //gl_Position = cam.uProj * vec4(inPos, 1.0);
//gl_Position = cam.uView * vec4(inPos, 1.0);
    mat3 N = mat3(W);
    vNrmW  = normalize(N * inNrm);
    vUV    = inUV;
    vPosW  = Pw.xyz;
}

#type fragment
#version 460

layout(location=0) in  vec3 vNrmW;
layout(location=1) in  vec2 vUV;
layout(location=2) in  vec3 vPosW;
layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform CameraUBO {
    mat4 uView;
    mat4 uProj;
} cam;

void main()
{
    vec3 N = normalize(vNrmW);
    // Quick directional light
    vec3 L = normalize(vec3(0.4, 0.8, 0.4));
    float ndl = max(dot(N, L), 0.0);
    vec3 base = vec3(0.8, 0.85, 0.9);
    // placeholder base color
    vec3 lit = base * (0.2 + 0.8 * ndl);
    outColor = vec4(lit, 1.0);
}
