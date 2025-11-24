#type vertex
#version 460

layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNrm;
layout(location=2) in vec2 inUV;

layout(location=0) out vec3 vNrmW;
layout(location=1) out vec2 vUV;
layout(location=2) out vec3 vPosW;

// Push constants: MUST be identical in both stages
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

// Per-instance data (vertex stage only)
struct Instance {
    mat4 world;
    mat4 worldPrev;
    uint materialId;
    uint boneBase;
    uint flags;
    uint objectId;
};

layout(std430, set=0, binding=1) readonly buffer InstanceData {
    Instance instances[];
} gInstances;

void main()
{
    Instance inst = gInstances.instances[pc.instanceIndex];

    mat4 W  = inst.world;
    mat4 VP = cam.uProj * cam.uView;

    vec4 Pw = W * vec4(inPos, 1.0);
    gl_Position = VP * Pw;

    mat3 N = mat3(W);
    vNrmW  = normalize(N * inNrm);
    vUV    = inUV;
    vPosW  = Pw.xyz;
}


#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require 

// Same push constant block as in vertex
layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialId;
    uint submeshId;
    uint flags;
} pc;

// One bindless-style array of albedo textures
layout(set=0, binding=2) uniform sampler2D uAlbedo;

struct Material {
    uint baseColorTex;
    uint normalTex;
    uint ormTex;
    uint emissiveTex;
    vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
    uint flags;
    uint _pad;
};




layout(std430, set=0, binding=3) readonly buffer MaterialBuffer {
    Material materials[];
} gMaterials;

layout(location=0) in  vec3 vNrmW;
layout(location=1) in  vec2 vUV;
layout(location=2) in  vec3 vPosW;
layout(location=0) out vec4 outColor;

void main()
{
    Material mat = gMaterials.materials[pc.materialId];

    vec3 N = normalize(vNrmW);
    vec3 L = normalize(vec3(0.4, 0.8, 0.4));
    float ndl = max(dot(N, L), 0.0);

    vec2 uv = clamp(vUV, 0.0, 1.0);
    vec4 texColor = texture(uAlbedo, uv);


    vec4 baseColor = texColor * mat.baseColorFactor;

    // simple lambert-ish light
    vec3 lit = baseColor.rgb * (0.2 + 0.8 * ndl);
    outColor = vec4(lit, baseColor.a);
}
