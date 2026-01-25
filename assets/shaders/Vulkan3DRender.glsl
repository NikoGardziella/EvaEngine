#type vertex
#version 460

layout(location = 0) in vec3  inPos;
layout(location = 1) in vec3  inNrm;
layout(location = 2) in vec2  inUV;
layout(location = 3) in uvec4 inJoints;
layout(location = 4) in vec4  inWeights;

layout(location = 0) out vec3 vNrmW;
layout(location = 1) out vec2 vUV;
layout(location = 2) out vec3 vPosW;

layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialId;
    uint submeshId;
    uint flags;
} pc;

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 uView;
    mat4 uProj;
} cam;

struct Instance {
    mat4 world;
    uint boneBase; 
    uint boneCount;
    uint _pad1;
    uint _pad2;
};

layout(std430, set = 0, binding = 1) readonly buffer InstanceData {
    Instance instances[];
} gInstances;

layout(std430, set = 0, binding = 4) readonly buffer BonePalette {
    mat4 uBones[];
};

void main()
{
    Instance inst = gInstances.instances[pc.instanceIndex];
    mat4 W = inst.world;
    
    vec4 localPos = vec4(inPos, 1.0);
    vec3 localNrm = inNrm;
    
    // Skinning if present
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
    
    vec4 Pw = W * localPos;
    gl_Position = cam.uProj * cam.uView * Pw;
    
    mat3 Nw = mat3(W);
    vNrmW = normalize(Nw * localNrm);
    vUV = inUV;
    vPosW = Pw.xyz;
}

#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialId;
    uint submeshId;
    uint flags;
} pc;

layout(set = 0, binding = 2) uniform sampler2D uAlbedo[];

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

layout(location = 0) in vec3 vNrmW;
layout(location = 1) in vec2 vUV;
layout(location = 2) in vec3 vPosW;

layout(location = 0) out vec4 outColor;

// ============================================================================
// LIGHTING SYSTEM - Same as 2D
// ============================================================================

struct GPUDirectionalLight {
    vec4 direction_intensity;
    vec4 color;
};

struct GPUPointLight {
    vec4 position_radius;
    vec4 color_intensity;
};

struct GPUSpotLight {
    vec4 position_range;
    vec4 direction_inner;
    vec4 color_outer;
    vec4 intensity_pad;
};

struct GPULightHeader {
    uint numDir;
    uint numPoint;
    uint numSpot;
    uint pad;
};

const uint MAX_DIR_LIGHTS = 1;
const uint MAX_POINT_LIGHTS = 64;
const uint MAX_SPOT_LIGHTS = 16;

layout(std430, set=0, binding=5) readonly buffer LightBuffer {
    GPULightHeader header;
    GPUDirectionalLight dir[MAX_DIR_LIGHTS];
    GPUPointLight point[MAX_POINT_LIGHTS];
    GPUSpotLight spot[MAX_SPOT_LIGHTS];
} lights;

vec3 calculateDirectionalLight(GPUDirectionalLight light, vec3 normal, vec3 albedo) {
    vec3 lightDir = normalize(-light.direction_intensity.xyz);
    float NdotL = max(dot(normal, lightDir), 0.0);
    return albedo * light.color.rgb * light.direction_intensity.w * NdotL;
}

vec3 calculatePointLight(GPUPointLight light, vec3 worldPos, vec3 normal, vec3 albedo) {
    vec3 toLight = light.position_radius.xyz - worldPos;
    float distance = length(toLight);
    float radius = light.position_radius.w;
    
    if (distance > radius) return vec3(0.0);
    
    vec3 lightDir = toLight / distance;
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    float attenuation = 1.0 - (distance / radius);
    attenuation = attenuation * attenuation;
    
    return albedo * light.color_intensity.rgb * light.color_intensity.w * NdotL * attenuation;
}

vec3 calculateSpotLight(GPUSpotLight light, vec3 worldPos, vec3 normal, vec3 albedo) {
    vec3 toLight = light.position_range.xyz - worldPos;
    float distance = length(toLight);
    float range = light.position_range.w;
    
    if (distance > range) return vec3(0.0);
    
    vec3 lightDir = toLight / distance;
    float NdotL = max(dot(normal, lightDir), 0.0);
    
    vec3 spotDir = normalize(-light.direction_inner.xyz);
    float cosTheta = dot(lightDir, spotDir);
    float cosInner = light.direction_inner.w;
    float cosOuter = light.color_outer.w;
    
    float spotEffect = smoothstep(cosOuter, cosInner, cosTheta);
    
    float attenuation = 1.0 - (distance / range);
    attenuation = attenuation * attenuation;
    
    return albedo * light.color_outer.rgb * light.intensity_pad.x * NdotL * attenuation * spotEffect;
}

vec3 applyLighting(vec3 worldPos, vec3 normal, vec3 albedo, float ambientStrength) {
    vec3 ambient = albedo * ambientStrength;
    vec3 lighting = ambient;
    
    for (uint i = 0; i < lights.header.numDir; ++i) {
        lighting += calculateDirectionalLight(lights.dir[i], normal, albedo);
    }
    
    for (uint i = 0; i < lights.header.numPoint; ++i) {
        lighting += calculatePointLight(lights.point[i], worldPos, normal, albedo);
    }
    
    for (uint i = 0; i < lights.header.numSpot; ++i) {
        lighting += calculateSpotLight(lights.spot[i], worldPos, normal, albedo);
    }
    
    return lighting;
}

void main()
{
    Material mat = gMaterials.materials[pc.materialId];
    vec3 normal = normalize(vNrmW);
    
    vec2 uv = clamp(vUV, 0.0, 1.0);
    uint texIndex = mat.baseColorTex;
    vec4 texColor = texture(uAlbedo[nonuniformEXT(texIndex)], uv);
    vec4 baseColor = texColor * mat.baseColorFactor;
    
    // Apply dynamic lighting
    vec3 litColor = applyLighting(vPosW, normal, baseColor.rgb, 0.2);
    
    outColor = vec4(litColor, baseColor.a);
}