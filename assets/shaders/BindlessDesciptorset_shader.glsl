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
layout(location=3) out        vec2 vWorldPos;
layout(location=4) out        vec4 vPosLightSpace;

layout(push_constant) uniform PC { 
    mat4 VP; 
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
    
    // CHANGE: Pivot at FEET (matches shadow pass)
    vec2 anchor = inst[i].worldPos; 
    vec2 local = vec2((q.x - 0.5) * inst[i].size.x, q.y * inst[i].size.y);
    
    float ang = inst[i].rotation;
    float c = cos(ang);
    float s = sin(ang);
    mat2 R = mat2(c, s, -s, c);
    
    vec2 pos = anchor + (R * local);
    vWorldPos = pos; 
    
    float pushBackY = 0.25;
    vec2 shadowPos = pos;
    shadowPos.y += pushBackY;

    gl_Position = pc.VP * vec4(pos, 0.0, 1.0); 

    // CALCULATE Shadow coordinates 
    // We use a small lookupBias (0.05) to look slightly "above" the floor
    // so the tile doesn't shadow itself.
    float receiverZ = 0.0; 
    vPosLightSpace = pc.lightSpaceMatrix * vec4(shadowPos, 0.0, 1.0);

    // Pass metadata
    vSlot = inst[i].slot;
    vFlags = inst[i].flags;

    // UVs
    vec2 uvMin = vec2(inst[i].uvMin16) / 65535.0;
    vec2 uvMax = vec2(inst[i].uvMax16) / 65535.0;
    vec2 t = q;
    if ((inst[i].flags & 1u) != 0u) t.y = 1.0 - t.y;
    vUV = mix(uvMin, uvMax, t);
}

#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_EXT_scalar_block_layout : enable

layout(location=0) in  flat uint vSlot;
layout(location=1) in        vec2 vUV;
layout(location=2) in  flat uint vFlags;
layout(location=3) in        vec2 vWorldPos;
layout(location=4) in        vec4 vPosLightSpace;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D uTiles[];
layout(set=0, binding=3) uniform sampler2D uSprites[];
layout(set=0, binding=5) uniform sampler2D uShadowMap3D;
layout(set=0, binding=6) uniform sampler2D uShadowMapTiles;

// ============================================================================
// LIGHTING SYSTEM (unchanged)
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

vec3 calculatePointLight(GPUPointLight light, vec3 worldPos, vec3 normal, vec3 albedo)
{
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

vec3 calculateSpotLight(GPUSpotLight light, vec3 worldPos, vec3 normal, vec3 albedo)
{
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
    for (uint i = 0; i < lights.header.numDir; ++i)
        lighting += calculateDirectionalLight(lights.dir[i], normal, albedo);
    for (uint i = 0; i < lights.header.numPoint; ++i)
        lighting += calculatePointLight(lights.point[i], worldPos, normal, albedo);
    for (uint i = 0; i < lights.header.numSpot; ++i)
        lighting += calculateSpotLight(lights.spot[i], worldPos, normal, albedo);
    return lighting;
}

// ============================================================================
// SHADOW: 3D objects (with Y-sort check)
// ============================================================================
float ShadowFactor_3D(vec4 posLightSpace, float receiverY)
{
    vec3 proj = posLightSpace.xyz / posLightSpace.w;
    vec2 uv = proj.xy * 0.5 + 0.5;

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || proj.z > 1.0)
        return 1.0;

    float currentDepth = proj.z;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap3D, 0));
    float shadowSum = 0.0;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 data = texture(uShadowMap3D, uv + vec2(x, y) * texelSize).rg;
            float casterDepth = data.r;
            float casterY = data.g;

            // Object is behind receiver — no shadow
            if (receiverY - casterY < -0.1) {
                shadowSum += 1.0;
                continue;
            }

            float bias = 0.002;
            shadowSum += (currentDepth <= casterDepth + bias) ? 1.0 : 0.0;
        }
    }
    return shadowSum / 9.0;
}


float ShadowFactor_Tile(vec4 posLightSpace)
{
    vec3 proj = posLightSpace.xyz / posLightSpace.w;
    vec2 uv = proj.xy * 0.5 + 0.5;

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || proj.z > 1.0)
        return 1.0;

    float currentDepth = proj.z;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMapTiles, 0));
    float shadowSum = 0.0;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            float casterDepth = texture(uShadowMapTiles, uv + vec2(x, y) * texelSize).r;
            float bias = 0.002;
            shadowSum += (currentDepth <= casterDepth + bias) ? 1.0 : 0.0;
        }
    }
    return shadowSum / 9.0;
}


void main()
{
    bool isSprite = (vFlags & 1u) != 0u;
    vec4 base = isSprite
        ? texture(uSprites[nonuniformEXT(vSlot)], vUV)
        : texture(uTiles[nonuniformEXT(vSlot)], vUV);

    if (base.a <= 0.001) discard;

    vec3 normal = vec3(0.0, 0.0, 1.0);
    vec3 worldPos3D = vec3(vWorldPos, 0.0);

    vec3 litColor = applyLighting(worldPos3D, normal, base.rgb, 0.2);

    // Sample both shadow maps, darkest wins
    float shadow3D   = ShadowFactor_3D(vPosLightSpace, vWorldPos.y);
    float shadowTile = ShadowFactor_Tile(vPosLightSpace);
    float shadow     = min(shadow3D, shadowTile);

    vec3 ambient = base.rgb * 0.2;
    vec3 directional = litColor - ambient;
    vec3 finalColor = ambient + directional * shadow;

    outColor = vec4(finalColor, base.a);
}