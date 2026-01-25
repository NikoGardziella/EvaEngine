#type vertex
#version 460
#extension GL_EXT_nonuniform_qualifier : require

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

layout(location=0) out flat uint vSlot;
layout(location=1) out        vec2 vUV;
layout(location=2) out flat uint vFlags;
layout(location=3) out        vec2 vWorldPos;  // For lighting

layout(push_constant) uniform PC { mat4 VP; } pc;

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
    mat2 R    = mat2(c,  s, -s, c);
    vec2 rotated = R * local;
    vec2 pos     = center + rotated;
    
    vWorldPos = pos;  // Pass world position for lighting
    
    gl_Position = pc.VP * vec4(pos, 0.0, 1.0);
    
    vec2 uvMin = vec2(inst[i].uvMin16) / 65535.0;
    vec2 uvMax = vec2(inst[i].uvMax16) / 65535.0;
    vec2 t = q;
    if ((inst[i].flags & 1u) != 0u) t.y = 1.0 - t.y;
    vUV   = mix(uvMin, uvMax, t);
    vSlot  = inst[i].slot;
    vFlags = inst[i].flags;
}

#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable

layout(location=0) in  flat uint vSlot;
layout(location=1) in        vec2 vUV;
layout(location=2) in  flat uint vFlags;
layout(location=3) in        vec2 vWorldPos;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D uTiles[];
layout(set=0, binding=3) uniform sampler2D uSprites[];


// ============================================================================
// LIGHTING SYSTEM
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

layout(std430, set=0, binding=4) readonly buffer LightBuffer {
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
    bool isSprite = (vFlags & 1u) != 0u;
    vec4 base = isSprite
        ? texture(uSprites[nonuniformEXT(vSlot)], vUV)
        : texture(uTiles[nonuniformEXT(vSlot)], vUV);
    
    if (base.a <= 0.001) discard;
    
    // 2D sprites face the camera
    vec3 normal = vec3(0.0, 0.0, 1.0);
    vec3 worldPos3D = vec3(vWorldPos, 0.0);
    
    // Apply lighting with 20% ambient
    vec3 litColor = applyLighting(worldPos3D, normal, base.rgb, 0.2);
    
    outColor = vec4(litColor, base.a);
}