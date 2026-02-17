#type vertex
#version 460

layout(location = 0) in vec3  inPos;
layout(location = 3) in uvec4 inJoints;
layout(location = 4) in vec4  inWeights;

// Pass the World Y to the fragment shader
layout(location = 0) out float vWorldY;

struct Instance {
    mat4 world;
    uint boneBase;
    uint boneCount;
    uint _pad1;
    uint _pad2;
};

layout(std430, set = 0, binding = 1) readonly buffer InstanceData { Instance instances[]; } gInstances;
layout(std430, set = 0, binding = 4) readonly buffer BonePalette  { mat4 uBones[]; } uBP;

layout(push_constant) uniform PC {
    mat4 lightVP;
    uint instanceIndex;
} pc;

void main()
{
    Instance inst = gInstances.instances[pc.instanceIndex];
    vec4 localPos = vec4(inPos, 1.0);

    // Skinning Logic
    if (inst.boneBase != 0xFFFFFFFFu)
    {
        uint base = inst.boneBase;
        mat4 skin =
              inWeights.x * uBP.uBones[base + inJoints.x]
            + inWeights.y * uBP.uBones[base + inJoints.y]
            + inWeights.z * uBP.uBones[base + inJoints.z]
            + inWeights.w * uBP.uBones[base + inJoints.w];
        localPos = skin * localPos;
    }

    vec4 worldPos = inst.world * localPos;
    
    // Pass the actual Y position for the X-Ray occlusion check
    vWorldY = inst.world[3].y;

    gl_Position = pc.lightVP * worldPos;
}

#type fragment
#version 460

layout(location = 0) in float vWorldY;
layout(location = 0) out vec2 outData; // R: Depth, G: WorldY

void main()
{
    // R channel gets the standard 0-1 depth
    // G channel gets the World Y coordinate
    outData = vec2(gl_FragCoord.z, vWorldY);

}