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
    uint  direction;
    uint  _pad0;
    uvec2 uvMin16;
    uvec2 uvMax16;
    uvec2 opaqueMin16;
    uvec2 opaqueMax16;
};

layout(location=0) out flat uint  vSlot;
layout(location=1) out vec2       vUV;
layout(location=2) out flat uint  vFlags;
layout(location=3) out vec2       vWorldPos;
layout(location=4) out vec4       vPosLightSpace;
layout(location=5) out flat float vFootY;
layout(location=6) out vec2       vLocalPos;
layout(location=7) out flat uint  vDirection;

layout(push_constant) uniform PC {
    mat4 VP;
    mat4 lightSpaceMatrix;
} pc;

layout(std430, set=0, binding=2) readonly buffer Instances {
    Instance inst[];
};

const vec2 quad[4] = vec2[](
    vec2(0.0, 0.0), vec2(1.0, 0.0),
    vec2(0.0, 1.0), vec2(1.0, 1.0)
);

void main()
{
    uint i = gl_InstanceIndex;
    vec2 q = quad[gl_VertexIndex];

    vec2 anchor = inst[i].worldPos;
    vec2 local;
    if (inst[i].size.y < 0.5)
    {
        // Small object (projectile): center both axes
        local = vec2((q.x - 0.5) * inst[i].size.x, (q.y - 0.5) * inst[i].size.y);
    }
    else
    {
        // Tile: bottom-center anchor
        local = vec2((q.x - 0.5) * inst[i].size.x, q.y * inst[i].size.y);
    }

    float ang = inst[i].rotation;
    float c = cos(ang);
    float s = sin(ang);
    mat2 R = mat2(c, s, -s, c);

    vec2 pos = anchor + (R * local);

    gl_Position = pc.VP * vec4(pos, 0.0, 1.0);
    vPosLightSpace = pc.lightSpaceMatrix * vec4(pos, 0.0, 1.0);

    vSlot = inst[i].slot;
    vFlags = inst[i].flags;
    vWorldPos = pos;
    vFootY = inst[i].worldPos.y;
    vLocalPos = vec2(q.x - 0.5, q.y);
    vDirection = inst[i].direction;

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

layout(location=0) in flat uint  vSlot;
layout(location=1) in vec2       vUV;
layout(location=2) in flat uint  vFlags;
layout(location=3) in vec2       vWorldPos;
layout(location=4) in vec4       vPosLightSpace;
layout(location=5) in flat float vFootY;
layout(location=6) in vec2       vLocalPos;
layout(location=7) in flat uint  vDirection;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D uTiles[];
layout(set=0, binding=3) uniform sampler2D uSprites[];
layout(set=0, binding=5) uniform sampler2D uShadowMap3D;
layout(set=0, binding=6) uniform sampler2D uShadowMapTiles;

layout(std140, set=0, binding=7) uniform PlayerData {
    vec2  playerScreenPos;
    float playerFootY;
    float fadeRadius;
} player;

// ============================================================================
// LIGHTING
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

vec3 calculateDirectionalLight(GPUDirectionalLight light, vec3 normal, vec3 albedo)
{
    vec3 lightDir = normalize(-light.direction_intensity.xyz);
    float NdotL = max(dot(normal, lightDir), 0.0);
    return albedo * light.color.rgb * light.direction_intensity.w * NdotL;
}

vec3 calculatePointLight(GPUPointLight light, vec3 worldPos, vec3 normal, vec3 albedo)
{
    vec3 toLight = light.position_radius.xyz - worldPos;
    float distance = length(toLight);
    float radius = light.position_radius.w;

    if (distance > radius)
    {
        return vec3(0.0);
    }

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

    if (distance > range)
    {
        return vec3(0.0);
    }

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

vec3 applyLighting(vec3 worldPos, vec3 normal, vec3 albedo, float ambientStrength)
{
    vec3 ambient = albedo * ambientStrength;
    vec3 lighting = ambient;

    for (uint i = 0; i < lights.header.numDir; ++i)
    {
        lighting += calculateDirectionalLight(lights.dir[i], normal, albedo);
    }

    for (uint i = 0; i < lights.header.numPoint; ++i)
    {
        lighting += calculatePointLight(lights.point[i], worldPos, normal, albedo);
    }
    for (uint i = 0; i < lights.header.numSpot; ++i)
    {
        lighting += calculateSpotLight(lights.spot[i], worldPos, normal, albedo);
    }
    return lighting;
}

// ============================================================================
// SHADOWS
// ============================================================================

float ShadowFactor_3D(vec4 posLightSpace, float receiverFootY)
{
    vec3 proj = posLightSpace.xyz / posLightSpace.w;
    vec2 uv = proj.xy * 0.5 + 0.5;
    
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || proj.z > 1.0)
        return 1.0;


    if (receiverFootY < player.playerFootY + 0.5) {
        return 1.0; 
    }

    float currentDepth = proj.z;
    float casterDepth = texture(uShadowMap3D, uv).r;

    // Standard shadow check
    return (currentDepth <= casterDepth + 0.002) ? 1.0 : 0.0;
}



float ShadowFactor_Tile(vec4 posLightSpace, float receiverY)
{
    vec3 proj = posLightSpace.xyz / posLightSpace.w;
    vec2 uv = proj.xy * 0.5 + 0.5;
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || proj.z > 1.0)
        return 1.0;
    float currentDepth = proj.z;
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMapTiles, 0));
    float shadowSum = 0.0;
    for (int y = -1; y <= 1; ++y) 
    {
        for (int x = -1; x <= 1; ++x)
        {
            vec2 data = texture(uShadowMapTiles, uv + vec2(x, y) * texelSize).rg;
            if (data.g - receiverY < 10.5)
            {
                shadowSum += 1.0;
                continue;
            }
            shadowSum += (currentDepth <= data.r + 0.002) ? 1.0 : 0.0;
        }
    }
    return shadowSum / 9.0;
}

// ============================================================================
// OCCLUSION FADE
// ============================================================================

float directionToAngle(uint direction)
{
    switch (direction) {
        case 0: return radians(-30.0);
        case 1: return radians(30.0);
        case 2: return radians(150.0);
        case 3: return radians(150.0);
        case 4: return radians(0.0);
        case 5: return radians(0.0);
        default: return radians(0.0);
    }
}

bool shouldFadeOcclusion(vec2 localPos, uint direction)
{

    float dist = distance(gl_FragCoord.xy, player.playerScreenPos);
    bool inFront = vFootY < player.playerFootY;

    if (!inFront || dist >= player.fadeRadius)
    {
        return false;
    }

    float closeness = 1.0 - (dist / player.fadeRadius);
    float cutoff = 1.0 - closeness * 0.66;
    float tilt = sin(directionToAngle(direction)) * localPos.x * 0.3;

    return (localPos.y + tilt > cutoff);
}


float getOcclusionAlpha(vec2 localPos, uint direction)
{
    float dist = distance(gl_FragCoord.xy, player.playerScreenPos);
    
    // Check if tile is in front of the player
    bool inFront = vFootY < player.playerFootY;

    // If tile is behind player or far away, it's fully opaque
    if (!inFront || dist >= player.fadeRadius)
    {
        return 1.0;
    }
    // 1. Calculate closeness (0.0 at edge of circle, 1.0 at center)
    float closeness = 1.0 - (dist / player.fadeRadius);
    
    // 2. Determine the "Fade Zone"
    // Tilt helps align the fade with the perspective of the tile
    float tilt = sin(directionToAngle(direction)) * localPos.x * 0.3;
    
    // This creates a soft gradient based on the height (localPos.y)
    // As the player gets closer, the "hole" grows taller

    // Soft 
    //float holeStrength = smoothstep(0.0, 0.5, localPos.y + tilt - (1.0 - closeness));
    // Hard
    //float holeStrength = smoothstep(0.0, 0.05, localPos.y - (1.0 - closeness));
    // Hardest (binary cut)
    float holeStrength = localPos.y - (1.0 - closeness) > 0.0 ? 1.0 : 0.0;


    // We cap it at 0.2 so the wall never becomes completely invisible
    return mix(1.0, 0.2, holeStrength);
}

// ============================================================================
// MAIN
// ============================================================================

void main()
{


    bool isSprite = (vFlags & 1u) != 0u;
    vec4 base = isSprite
        ? texture(uSprites[nonuniformEXT(vSlot)], vUV)
        : texture(uTiles[nonuniformEXT(vSlot)], vUV);

    // Keep the discard for the actual texture transparency (holes in the sprite)
    if (base.a <= 0.001) discard;

    // --- Occlusion Fade Logic ---
    float occlusionAlpha = 1.0;
    if (!isSprite) {
        occlusionAlpha = getOcclusionAlpha(vLocalPos, vDirection);
    }

    // --- Lighting Logic ---
    vec3 normal = vec3(0.0, 0.0, 1.0);
    vec3 worldPos3D = vec3(vWorldPos, 0.0);
    vec3 litColor = applyLighting(worldPos3D, normal, base.rgb, 0.2);


    float shadow3D   = ShadowFactor_3D(vPosLightSpace, vFootY);
    float shadowTile = ShadowFactor_Tile(vPosLightSpace, vFootY);
    float shadow     = min(shadow3D, shadowTile);

    vec3 ambient = base.rgb * 0.2;
    vec3 finalColor = ambient + (litColor - ambient) * shadow;

    // --- Final Output ---
    // Multiply the texture's original alpha by our occlusion fade
    outColor = vec4(finalColor, base.a * occlusionAlpha);
}