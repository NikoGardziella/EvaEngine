#type vertex
#version 450
layout(location = 0) in vec2 inPos;
layout(location = 0) out vec2 fragWorldPos;
layout(location = 1) out vec2 fragScreenPos;

layout(push_constant) uniform PC {
    mat4 uVP;
    vec2 playerPos;
    float visRadius;
    float time;
} pc;

void main()
{
    // Check if this is the screen-space quad (NDC coordinates)
    if (abs(inPos.x) <= 1.01 && abs(inPos.y) <= 1.01)
    {
        gl_Position = vec4(inPos.xy, 0.0, 1.0);
        fragScreenPos = inPos;
        
        // We need actual world position - calculate from inverse VP
        // Since inverse() is expensive, let's use screen position to derive it
        fragWorldPos = vec2(0.0); // Will use screen pos in fragment shader
    }
    else
    {
        // World-space geometry (visibility fan)
        fragWorldPos = inPos;
        fragScreenPos = (pc.uVP * vec4(inPos, 0.0, 1.0)).xy;
        gl_Position = pc.uVP * vec4(inPos.xy, 0.0, 1.0);
    }
}

#type fragment
#version 450
layout(location = 0) in vec2 fragWorldPos;
layout(location = 1) in vec2 fragScreenPos;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PC {
    mat4 uVP;
    vec2 playerPos;
    float visRadius;
    float time;
} pc;

float hash(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    
    // Quintic interpolation (smoother than cubic)
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
    // For screen-space quad, use screen position to generate world-like coordinates
    // This creates variation across the screen
    vec2 samplePos = fragScreenPos * 20.0 + pc.playerPos * 0.5;
    
    // Multiple animated noise layers
    float t = pc.time;
    vec2 uv1 = samplePos * 0.3 + vec2(t * 0.1, t * 0.08);
    vec2 uv2 = samplePos * 0.5 + vec2(-t * 0.15, t * 0.12);
    vec2 uv3 = samplePos * 1.0 + vec2(t * 0.05, -t * 0.07);
    
    float n1 = fbm(uv1);
    float n2 = fbm(uv2);
    float n3 = noise(uv3);
    
    // Create dramatic wispy patterns
    float wisps = pow(abs(n1 - n2), 1.5);
    float detail = n3;
    
    // Combine for layered effect
    float fogPattern = n1 * 0.4 + wisps * 0.4 + detail * 0.2;
    
    // Create variation in density
    float density = smoothstep(0.3, 0.7, fogPattern);
    
    // Color variation based on pattern
    vec3 color1 = vec3(0.01, 0.01, 0.02);  // Very dark
    vec3 color2 = vec3(0.04, 0.05, 0.08);  // Dark blue-grey
    vec3 color3 = vec3(0.08, 0.09, 0.12);  // Lighter grey
    
    vec3 fogColor = mix(color1, color2, fogPattern);
    fogColor = mix(fogColor, color3, wisps);
    
    // Add some brightness variation
    float brightness = 0.85 + sin(t * 0.3 + fogPattern * 6.28) * 0.1;
    
    // Final alpha with more variation
    float alpha = 0.5 + density * 0.4;
    alpha *= brightness;
    alpha = clamp(alpha, 0.4, 0.95);
    
    outColor = vec4(fogColor, alpha);
}