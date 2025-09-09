#type vertex
#version 460
#extension GL_EXT_nonuniform_qualifier : require

// Per-instance data (must match your CPU struct layout)
struct Instance {
    vec2  worldPos;  // world-space center
    vec2  size;      // width, height (world units)
    float zSortKey;  // not used by depth; you sort on CPU
    uint  slot;      // bindless texture slot
    uint  flags;
    uint  _pad;
};

layout(location=0) out flat uint vSlot;
layout(location=1) out vec2      vUV;

layout(push_constant) uniform PC { mat4 VP; } pc;

// SSBO: set=0, binding=2 (bindless layout)
layout(std430, set=0, binding=2) readonly buffer Instances {
    Instance inst[];
};

const vec2 quad[4] = vec2[](
    vec2(0.0,0.0), vec2(1.0,0.0),
    vec2(0.0,1.0), vec2(1.0,1.0)
);

void main() {
    uint i = gl_InstanceIndex;
    vec2 q = quad[gl_VertexIndex];     // 0..1

    // Convert bottom-center pivot (ground) to center pivot
    vec2 center = inst[i].worldPos + vec2(0.0, 0.5 * inst[i].size.y);

    // Build the quad about the center
    vec2 pos = center + (q - vec2(0.5)) * inst[i].size;

    gl_Position = pc.VP * vec4(pos, 0.0, 1.0);

    // Flip V (old path used flipped UVs)
    vUV   = vec2(q.x, 1.0 - q.y);
    vSlot = inst[i].slot;
}

#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in  flat uint vSlot;
layout(location=1) in        vec2 vUV;
layout(location=0) out       vec4 outColor;

// Bindless arrays (match your descriptor set 0 layout)
layout(set=0, binding=0) uniform sampler2D uColor[];          // COMBINED_IMAGE_SAMPLER[]
// Optional health/damage mask per tile layer (rgba8 storage)
#ifdef USE_HEALTH
layout(set=0, binding=1, rgba8) uniform image2D uHealth[];    // STORAGE_IMAGE[]
#endif

void main()
{
    vec4 base = texture(uColor[nonuniformEXT(vSlot)], vUV);
    if (base.a <= 0.001) discard;

#ifdef USE_HEALTH
    // Modulate alpha by a per-pixel health mask (R channel), if you keep one
    ivec2 ts = textureSize(uColor[nonuniformEXT(vSlot)], 0);
    ivec2 p  = ivec2(vUV * vec2(ts));
    float hp = imageLoad(uHealth[nonuniformEXT(vSlot)], p).r;  // 0..1
    base.a *= clamp(hp, 0.0, 1.0);
    // Optional scorch tint:
    // base.rgb = mix(base.rgb, vec3(0.35, 0.25, 0.15), 1.0 - hp);
#endif

    outColor = base;
}
