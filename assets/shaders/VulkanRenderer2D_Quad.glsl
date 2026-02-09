#type vertex
#version 450 core

layout(location = 0) in vec3  a_Position;
layout(location = 1) in vec4  a_Color;
layout(location = 2) in vec2  a_TexCoord;
layout(location = 3) in uint  a_TexIndex;
layout(location = 4) in float a_TilingFactor;

layout(std140, set = 0, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
};

layout(location = 0) out vec4  v_Color;
layout(location = 1) out vec2  v_TexCoord;
layout(location = 2) out float v_TilingFactor;
layout(location = 3) out flat uint v_TexIndex;
layout(location = 4) out vec3  v_WorldPos;
layout(location = 5) out vec4  v_PosLightSpace;

layout(push_constant) uniform PC {
    mat4 lightSpaceMatrix; // this is only using mat4
} pc;

void main()
{
    v_Color        = a_Color;
    v_TexCoord     = a_TexCoord;
    v_TilingFactor = a_TilingFactor;
    v_TexIndex     = a_TexIndex;
    v_WorldPos     = a_Position;

    // 1. Render to screen using Camera
    gl_Position    = u_ViewProjection * vec4(a_Position, 1.0);

    // 2. Project ground into Shadow Space
    // IMPORTANT: If your tiles are at Z=5.0, your ground is likely at Z=0.0.
    // Ensure this Z (0.0) is what the shadow pass expects for the ground plane.
    v_PosLightSpace = pc.lightSpaceMatrix * vec4(a_Position.xy, 0.0, 1.0);
}

#type fragment
#version 450 core
#extension GL_EXT_scalar_block_layout : enable

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec4  v_Color;
layout(location = 1) in vec2  v_TexCoord;
layout(location = 2) in float v_TilingFactor;
layout(location = 3) in flat uint v_TexIndex;
layout(location = 4) in vec3  v_WorldPos;
layout(location = 5) in vec4  v_PosLightSpace;

layout(set = 1, binding = 1) uniform sampler2D u_Textures[32];
layout(set = 0, binding = 2) uniform sampler2D uShadowMap;

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

layout(std430, set=0, binding=1) readonly buffer LightBuffer {
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

// ============================================================================
// SHADOW SYSTEM
// ============================================================================

float ShadowFactor(vec4 posLightSpace)
{
    vec3 proj = posLightSpace.xyz / posLightSpace.w;
    vec2 uv = proj.xy * 0.5 + 0.5;
    
    // VULKAN Y-FLIP: Ensure this matches your Tile shader logic

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || proj.z > 1.0)
        return 1.0;
    
    float currentDepth = proj.z;
    float bias = 0.0005;
    
    vec2 texelSize = 1.0 / vec2(textureSize(uShadowMap, 0));
    float shadow = 0.0;
    
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            float shadowDepth = texture(uShadowMap, uv + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > shadowDepth) ? 0.0 : 1.0;
        }
    }
    
    return shadow / 9.0;
}

float ShadowFactor_PCF(vec4 posLightSpace)
{
    vec3 proj = posLightSpace.xyz / posLightSpace.w;
    vec2 uv = proj.xy * 0.5 + 0.5;
    uv.y = 1.0 - uv.y; // Standard Vulkan flip

    // If outside the shadow frustum, return "Lit" (1.0)
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0 || proj.z > 1.0) 
        return 1.0;

    float currentDepth = proj.z;
    
    // REDUCE BIAS: If it's too high, it "pushes" the shadow through the floor
    float bias = 0.0001; 
    
    // Simple 1-tap check first to see if it works
    float shadowDepth = texture(uShadowMap, uv).r;
    
    // If current point is further than the map, it's 0.3 (dark), else 1.0 (bright)

    float shadowStrength = 0.05;
    return (currentDepth - bias > shadowDepth) ? shadowStrength : 1.0;
}


void main()
{

    // DEBUG: Force visualize the shadow map on the ground
    vec3 proj = v_PosLightSpace.xyz / v_PosLightSpace.w;
    vec2 uv = proj.xy * 0.5 + 0.5;
    float d = texture(uShadowMap, uv).r;

   // o_Color = vec4(vec3(d), 1.0); 
   // return;

    // 1. Base Texture Lookup
    uint idx = min(v_TexIndex, 31u);
    vec4 tex = texture(u_Textures[idx], v_TexCoord * v_TilingFactor);
    if (tex.a < 0.1) discard;

    // 2. Lighting & Normal Setup
    vec3 normal = vec3(0.0, 0.0, 1.0); // 2D flat ground
    vec3 litColor = applyLighting(v_WorldPos, normal, tex.rgb, 0.2);
    
    // 3. Shadow Calculation
    float shadow = ShadowFactor(v_PosLightSpace);
    
    // 4. Combine: Only apply shadow to the "Direct" lighting, not Ambient
    vec3 ambient = tex.rgb * 0.2;
    vec3 direct = litColor - ambient;
    
    // final color = ambient + shadowed direct light
    vec3 finalColor = ambient + (direct * shadow);
    
    o_Color = vec4(finalColor, tex.a);
}