#type vertex
#version 450

layout(location = 0) in vec2 inPos;          // WORLD position (fan verts or quad verts)
layout(location = 0) out vec2 fragWorldPos;
layout(location = 1) out vec2 vNdc;


layout(push_constant) uniform PC {
    mat4 uVP;
    mat4 uInvVP;     // can keep for now, but unused
    vec2 playerPos;
    float visRadius;
    float time;
    uint flags;
} pc;

void main()
{
    if ((pc.flags & 1u) != 0u) {
        gl_Position = vec4(inPos, 0.0, 1.0);
        vNdc = inPos;
        fragWorldPos = vec2(0.0);
    } else {
        fragWorldPos = inPos;
        vec4 clip = pc.uVP * vec4(inPos, 0.0, 1.0);
        gl_Position = clip;
        vNdc = clip.xy / clip.w; // not needed, but ok
    }
}

#type fragment
#version 450

layout(location = 0) in vec2 fragWorldPos;
layout(location = 1) in vec2 vNdc;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PC {
    mat4 uVP;
    mat4 uInvVP;     // unused
    vec2 playerPos;
    float visRadius;
    float time;
    uint flags;
} pc;


vec3 Unproject(vec2 ndcXY, float ndcZ)
{
    // Vulkan NDC: z in [0,1]
    vec4 ndc = vec4(ndcXY, ndcZ, 1.0);
    vec4 w = pc.uInvVP * ndc;
    return w.xyz / max(abs(w.w), 1e-6);
}

vec2 WorldFromNdc(vec2 ndcXY)
{
    vec3 nearW = Unproject(ndcXY, 0.0);
    vec3 farW  = Unproject(ndcXY, 1.0);
    vec3 dir = farW - nearW;

    // intersect with z=0 plane
    float t = 0.0;
    if (abs(dir.z) > 1e-6) t = (0.0 - nearW.z) / dir.z;
    vec3 p = nearW + t * dir;
    return p.xy;
}

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;

    for (int i = 0; i < 5; i++) {
        value += amplitude * noise(p);
        p *= 2.0;
        amplitude *= 0.5;
    }
    return value;
}


void main()
{
    // World position directly from vertex
    vec2 worldPos = ((pc.flags & 1u) != 0u) ? WorldFromNdc(vNdc) : fragWorldPos;

      
    
    // Fog pattern in WORLD space
    vec2 samplePos = worldPos * 0.5;

    float t = pc.time;
    vec2 uv1 = samplePos * 0.30 + vec2(t * 0.10,  t * 0.08);
    vec2 uv2 = samplePos * 0.55 + vec2(-t * 0.15, t * 0.12);
    vec2 uv3 = samplePos * 1.10 + vec2(t * 0.05, -t * 0.07);

    float n1 = fbm(uv1);
    float n2 = fbm(uv2);
    float n3 = noise(uv3);

    float wisps = pow(abs(n1 - n2), 1.5);
    float fogPattern = n1 * 0.4 + wisps * 0.4 + n3 * 0.2;
    float density = smoothstep(0.3, 0.7, fogPattern);

    vec3 color1 = vec3(0.01, 0.01, 0.02);
    vec3 color2 = vec3(0.04, 0.05, 0.08);
    vec3 color3 = vec3(0.08, 0.09, 0.12);

    vec3 fogColor = mix(color1, color2, fogPattern);
    fogColor = mix(fogColor, color3, wisps);

    float brightness = 0.85 + sin(t * 0.3 + fogPattern * 6.2831853) * 0.1;

    float alpha = 0.5 + density * 0.4;
    alpha *= brightness;

    // ===================================================================
    // Distance calculation in SCREEN SPACE to match projected oval shape
    // ===================================================================
    
    // Project fragment world position to screen space
    vec4 fragClip = pc.uVP * vec4(worldPos, 0.0, 1.0);
    vec2 fragScreen = fragClip.xy / max(abs(fragClip.w), 1e-6);
    
    // Project player position to screen space
    vec4 playerClip = pc.uVP * vec4(pc.playerPos, 0.0, 1.0);
    vec2 playerScreen = playerClip.xy / max(abs(playerClip.w), 1e-6);
    
    // Distance in screen space
    float screenDist = length(fragScreen - playerScreen);
    
    // Project the visibility radius to screen space
    vec4 radiusClip = pc.uVP * vec4(pc.playerPos + vec2(pc.visRadius, 0.0), 0.0, 1.0);
    vec2 radiusScreen = radiusClip.xy / max(abs(radiusClip.w), 1e-6);
    float screenRadius = length(radiusScreen - playerScreen);
    
    // Normalized distance (1.0 = at the edge of visibility)
    float normalizedDist = screenDist / max(screenRadius, 1e-6);

    // Fade at edge of visibility (oval-shaped due to camera projection)
    //float edgeFade = smoothstep(0.95, 1.05, normalizedDist);
    //float distanceFromEdge = abs(normalizedDist - 1.0);
   // float isNearEdge = 1.0 - smoothstep(0.0, 0.15, distanceFromEdge);

    //alpha = mix(alpha, alpha * edgeFade, isNearEdge);
    alpha = clamp(alpha, 0.0, 0.95);

    outColor = vec4(fogColor, alpha);
}