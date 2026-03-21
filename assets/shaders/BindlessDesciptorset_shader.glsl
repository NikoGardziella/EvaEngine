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
layout(location=8) out flat float vZkey;

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

    //if ((inst[i].flags & 1u) != 0u) t.y = 1.0 - t.y;

    vUV = mix(uvMin, uvMax, t);

    vZkey = inst[i].zSortKey;
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
layout(location=8) in flat float vZkey;

layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D uTiles[];
layout(set=0, binding=3) uniform sampler2D uSprites[];
layout(set=0, binding=5) uniform sampler2D uShadowMap3D;
layout(set=0, binding=6) uniform sampler2D uShadowMapTiles;

layout(std140, set=0, binding=7) uniform PlayerData {
    vec2  playerPos;
    float playerFootY;
    float fadeRadius;
    float visRadius;
    vec2  screenSize;
    vec2  _pad;
} player;


layout(set = 0, binding = 8) uniform sampler2D uVisibilityMap;



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

    float dist = distance(gl_FragCoord.xy, vec2(0, 0));
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


float getOcclusionAlpha(vec2 worldPos, uint direction)
{
    float fadeRadius = player.fadeRadius;
    
    // Check if Bit 1 (Value 2) is set in vFlags for Roof
    bool isRoof = (vFlags & 2u) == 1u;

    bool playerIsInsideEntityArea = (vFlags & 3u) == 1u;

    if(playerIsInsideEntityArea)
    {
        return 1.0;
    }

    return 0.1;

    if (isRoof) 
    {
        fadeRadius *= 50.0; 
        // Offset the sampling down to match player feet level
        worldPos.y -= 100.5; 
    }

    float dist = distance(worldPos, player.playerPos);
    
    // ROOF LOGIC: Simple radial fade
    if (isRoof)
    {
        // Use vFootY (the ground) for distance, NOT the pixel worldPos
        float distToGround = distance(vec2(vWorldPos.x, vFootY), player.playerPos);
    
        // If player is within 2 tiles of the building base, fade the WHOLE roof
        return smoothstep(player.fadeRadius, player.fadeRadius + 1.0, distToGround) * 0.8 + 0.2;
    }

    // WALL/NORMAL TILE LOGIC
   // bool inFront = vFootY < player.playerFootY;
   // if (!inFront || dist >= fadeRadius) return 1.0;

    float closeness = 1.0 - (dist / fadeRadius);
    float tilt = sin(directionToAngle(direction)) * vLocalPos.x * 0.3;
    float holeStrength = (vLocalPos.y + tilt) - (1.0 - closeness) > 0.0 ? 1.0 : 0.0;

    return mix(1.0, 0.2, holeStrength);
}


float sampleVisibility(vec2 worldPos)
{
    vec2 mapMin  = player.playerPos - vec2(player.visRadius);
    vec2 mapSize = vec2(player.visRadius * 2.0);

    vec2 uv = (worldPos - mapMin) / mapSize;
    uv.y = 1.0 - uv.y;
    // Check bounds
    if (any(lessThan(uv, vec2(0.0))) || any(greaterThan(uv, vec2(1.0))))
        return 0.0;

    return texture(uVisibilityMap, uv).r;
}

vec2 getFogUV(vec2 worldPos, vec2 playerPos, float visRadius)
{
    vec2 mapMin  = playerPos - vec2(visRadius);
    vec2 mapSize = vec2(visRadius * 2.0);
    return (worldPos - mapMin) / mapSize;
}


float getDilatedVisibility(vec2 worldPos) 
{
    // How far out to look for a "visible" neighbor (in world units)
    // Increase this if your roofs are still partially popping
    float searchRadius = 1.5; 
    
    float maxVis = 0.0;
    
    // Check center + 4 diagonal points
    // This is cheap and usually enough to cover a tile's footprint
    vec2 offsets[5] = vec2[](
        vec2(0.0, 0.0),
        vec2(-searchRadius, -searchRadius),
        vec2(searchRadius, -searchRadius),
        vec2(-searchRadius, searchRadius),
        vec2(searchRadius, searchRadius)
    );

    for(int i = 0; i < 5; i++)
    {
        vec2 uv = getFogUV(worldPos + offsets[i], player.playerPos, player.visRadius);

        if (all(greaterThanEqual(uv, vec2(0.0))) && all(lessThanEqual(uv, vec2(1.0))))
        {
            maxVis = max(maxVis, texture(uVisibilityMap, uv).r);
        }
    }
    
    return maxVis;
}

float getTileVisibility(vec2 worldPos) 
{
    float sum = 0.0;
    float samples = 0.0;
    // Check a 3x3 grid around the tile center
    for(float y = -1.0; y <= 1.0; y += 1.0) {
        for(float x = -1.0; x <= 1.0; x += 1.0) {
            vec2 uv = getFogUV(worldPos + vec2(x, y) * 0.5, player.playerPos, player.visRadius);
            sum += texture(uVisibilityMap, uv).r;
            samples += 1.0;
        }
    }
    return sum / samples; // Returns a smooth 0.0 to 1.0
}

// ============================================================================
// MAIN
// ============================================================================

void main()
{
    // 1. Texture Sampling
    bool isSprite = (vFlags & 1u) != 0u;
    vec4 base = isSprite
        ? texture(uSprites[nonuniformEXT(vSlot)], vUV)
        : texture(uTiles[nonuniformEXT(vSlot)], vUV);

    if (base.a <= 0.001) discard;

    // 2. Flags and Visibility Setup
    bool isRoof = (vFlags & 2u) != 0u;


    bool playerInsideEntityArea = (vFlags & 4u) != 0u;
   

    vec2 tilePos = vWorldPos;
    float visibility = 0.0;

    if(playerInsideEntityArea)
    {
        visibility = 0.1;
    }

    if (isRoof)
    {
        // Sample slightly lower so roof visibility matches the ground it covers
        tilePos.y -= 0.5; 
        // Use dilated check to keep the whole roof rendered together
        //visibility = getDilatedVisibility(tilePos);
    }
    else
    {
        // Standard check for floor/walls
        vec2 fogUV = getFogUV(tilePos, player.playerPos, player.visRadius);
        // Check bounds before sampling
        if (all(greaterThanEqual(fogUV, vec2(0.0))) && all(lessThanEqual(fogUV, vec2(1.0))))
        {
           // visibility = texture(uVisibilityMap, fogUV).r;
        }
    }

    // 3. Lighting Calculation
    vec3 normal = vec3(0.0, 0.0, 1.0);
    vec3 worldPos3D = vec3(vWorldPos, 0.0);
    vec3 litColor = applyLighting(worldPos3D, normal, base.rgb, 0.2);

    // 4. Shadow Calculation
    float shadow3D   = ShadowFactor_3D(vPosLightSpace, vFootY);
    float shadowTile = ShadowFactor_Tile(vPosLightSpace, vFootY);
    float shadow     = min(shadow3D, shadowTile);

    vec3 ambient    = base.rgb * 0.2;
    vec3 finalColor = ambient + (litColor - ambient) * shadow;

    // 5. Occlusion Calculation
    // Note: Use vWorldPos here to ensure distance math matches playerPos scale
    float occ = getOcclusionAlpha(vWorldPos, vDirection);
    
    // Only apply the "hole" where the player actually has vision
    float finalOcclusionAlpha = mix(1.0, occ, visibility);

    float occulsonAlpha = 1.0;
    if(playerInsideEntityArea)
    {
        occulsonAlpha  = 0.1;
    }

    outColor = vec4(finalColor, base.a * occulsonAlpha);
}