#type vertex
#version 460

layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNrm;
layout(location=2) in vec2 inUV;
layout(location=3) in uvec4 inJoints;   // JOINTS_0
layout(location=4) in vec4  inWeights;  // WEIGHTS_0

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
    uint boneBase; 
    uint boneCount;
    uint _pad1;
    uint _pad2;
};

layout(std430, set=0, binding=1) readonly buffer InstanceData {
    Instance instances[];
} gInstances;

// Bone palette: all bones for all skeletons live here
layout(std430, set=0, binding=4) readonly buffer BonePalette {
    mat4 uBones[];
};

void main()
{
    Instance inst = gInstances.instances[pc.instanceIndex];

    mat4 W  = inst.world;
    mat4 VP = cam.uProj * cam.uView;

    // Start with local position/normal
    vec4 localPos  = vec4(inPos, 1.0);
    vec3 localNrm  = inNrm;

    // Apply skinning if this instance has a valid boneBase
   
   
    if (inst.boneBase != 0xFFFFFFFFu)
    {
        uint base = inst.boneBase;

        mat4 skinMat =
              inWeights.x * uBones[base + inJoints.x]
            + inWeights.y * uBones[base + inJoints.y]
            + inWeights.z * uBones[base + inJoints.z]
            + inWeights.w * uBones[base + inJoints.w];

        localPos = skinMat * localPos;
        localNrm = mat3(skinMat) * localNrm;
    }


    // World-space transform
    vec4 Pw = W * localPos;
    gl_Position = VP * Pw;

    mat3 Nw = mat3(W);
    vNrmW  = normalize(Nw * localNrm);
    vUV    = inUV;
    vPosW  = Pw.xyz;

    vec4 w = inWeights;
    float wSum = w.x + w.y + w.z + w.w;
    if (wSum > 0.00001) w /= wSum;

    uint j = inJoints.x;
    float bestW = w.x;
    if (w.y > bestW) { bestW = w.y; j = inJoints.y; }
    if (w.z > bestW) { bestW = w.z; j = inJoints.z; }
    if (w.w > bestW) { bestW = w.w; j = inJoints.w; }

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

// Albedo texture (right now a single sampler; you can later make it an array)
layout(set = 0, binding = 2) uniform sampler2D uAlbedo;

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

layout(std430, set = 0, binding = 3) readonly buffer MaterialBuffer {
    Material materials[];
} gMaterials;

layout(location = 0) in  vec3 vNrmW;
layout(location = 1) in  vec2 vUV;
layout(location = 2) in  vec3 vPosW;
// location = 3 (vBoneIdNorm) can exist in VS, but we just don't use it here

layout(location = 0) out vec4 outColor;

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
