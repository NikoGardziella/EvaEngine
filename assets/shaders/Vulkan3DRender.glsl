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
layout(location = 3) out vec4 vPosLightSpace;

layout(push_constant) uniform PC {
    uint instanceIndex;
    uint materialId;
    uint submeshId;
    uint flags;
    mat4 lightSpaceMatrix; // lightProj * lightView (NO CPU bias)
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

    gl_Position     = cam.uProj * cam.uView * Pw;
    vPosW           = Pw.xyz;
    vUV             = inUV;

    mat3 Nw = mat3(W);
    vNrmW = normalize(Nw * localNrm);

    vPosLightSpace = pc.lightSpaceMatrix * Pw;
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
layout(location = 3) in vec4 vPosLightSpace;

layout(location = 0) out vec4 outColor;

// IMPORTANT: unify across all passes
layout(set = 0, binding = 6) uniform sampler2D uShadowMap;

// lights buffer (your layout)
struct GPUDirectionalLight { vec4 direction_intensity; vec4 color; };
struct GPUPointLight       { vec4 position_radius;     vec4 color_intensity; };
struct GPUSpotLight        { vec4 position_range;      vec4 direction_inner; vec4 color_outer; vec4 intensity_pad; };
struct GPULightHeader      { uint numDir; uint numPoint; uint numSpot; uint pad; };

const uint MAX_DIR_LIGHTS   = 1;
const uint MAX_POINT_LIGHTS = 64;
const uint MAX_SPOT_LIGHTS  = 16;

layout(std430, set=0, binding=5) readonly buffer LightBuffer {
    GPULightHeader header;
    GPUDirectionalLight dir[MAX_DIR_LIGHTS];
    GPUPointLight point[MAX_POINT_LIGHTS];
    GPUSpotLight spot[MAX_SPOT_LIGHTS];
} lights;

vec3 calculateDirectionalLight(GPUDirectionalLight light, vec3 normal, vec3 albedo)
{
    vec3 lightDir = normalize(-light.direction_intensity.xyz);
    float NdotL = max(dot(normal, lightDir), 0.0);
    return albedo * light.color.rgb * light.direction_intensity.w * NdotL;
}

vec3 applyLighting(vec3 worldPos, vec3 normal, vec3 albedo, float ambientStrength)
{
    vec3 ambient = albedo * ambientStrength;
    vec3 lighting = ambient;

    for (uint i = 0; i < lights.header.numDir; ++i)
        lighting += calculateDirectionalLight(lights.dir[i], normal, albedo);

    // Keep your point/spot loops here if you want
    return lighting;
}

float ShadowFactor(vec4 posLightSpace, vec3 normalWS, vec3 lightDirWS)
{
    vec3 proj = posLightSpace.xyz / posLightSpace.w;
    vec2 uv   = proj.xy * 0.5 + 0.5;

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
        return 1.0;

    float z = proj.z;
    if (z < 0.0 || z > 1.0)
        z = z * 0.5 + 0.5;
    if (z < 0.0 || z > 1.0)
        return 1.0;

    // Slope-scaled bias (3D-friendly)
    float ndotl = clamp(dot(normalize(normalWS), normalize(-lightDirWS)), 0.0, 1.0);
    float bias  = max(0.0008 * (1.0 - ndotl), 0.0004);

    vec2 texel = 1.0 / vec2(textureSize(uShadowMap, 0));
    float sum = 0.0;

    for (int y = -1; y <= 1; y++)
    for (int x = -1; x <= 1; x++)
    {
        float closest = texture(uShadowMap, uv + vec2(x,y) * texel).r;
        sum += (z - bias > closest) ? 0.0 : 1.0;
    }

    return sum / 9.0;
}


void main()
{

    Material mat = gMaterials.materials[pc.materialId];

    vec2 uv = clamp(vUV, 0.0, 1.0);
    vec4 baseColor;
    if (mat.baseColorTex == 0xFFFFFFFFu)
    {
        baseColor = mat.baseColorFactor;
    }
    else
    {
        baseColor = texture(uAlbedo[nonuniformEXT(mat.baseColorTex)], uv) * mat.baseColorFactor;
    }
    vec3 normal = normalize(vNrmW);

    // Ambient split, so only direct light gets shadowed
    float ambientStrength = 0.2;
    vec3 ambient = baseColor.rgb * ambientStrength;

    vec3 lit = applyLighting(vPosW, normal, baseColor.rgb, ambientStrength);

    // We only shadow the first directional light (simple start)
    vec3 dirContrib = lit - ambient;

    vec3 lightDirWS = vec3(0.0, 1.0, 0.0);
    if (lights.header.numDir > 0)
        lightDirWS = normalize(-lights.dir[0].direction_intensity.xyz);

    float shadow = ShadowFactor(vPosLightSpace, vec3(0,0,1), lights.dir[0].direction_intensity.xyz);

    vec3 finalColor = ambient + dirContrib * shadow;
    outColor = vec4(finalColor, baseColor.a);
}
