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
    uint flags;
} pc;

void main()
{
    if ((pc.flags & 1u) != 0u)
    {
        gl_Position = vec4(inPos, 0, 1);
        fragScreenPos = inPos;
        fragWorldPos = vec2(0);
    }
    else
    {
        vec4 clip = pc.uVP * vec4(inPos, 0, 1);
        gl_Position = clip;
        fragWorldPos = inPos;
        fragScreenPos = clip.xy / clip.w;
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
    uint flags;
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
    vec2 uv = fragScreenPos * 0.5 + 0.5; // 0..1

    // screen-space noise domain
    vec2 samplePos = uv * 12.0 + pc.playerPos * 0.02; // optional world anchoring

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

    float brightness = 0.85 + sin(t * 0.3 + fogPattern * 6.28) * 0.1;

    float alpha = 0.5 + density * 0.4;
    alpha *= brightness;
    alpha = clamp(alpha, 0.4, 0.95);

    outColor = vec4(fogColor, alpha);
}
