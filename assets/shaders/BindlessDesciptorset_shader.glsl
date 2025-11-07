#type vertex
#version 460
#extension GL_EXT_nonuniform_qualifier : require

// Per-instance data (must match CPU std430 layout)
struct Instance {
    vec2  worldPos;   // world-space ground point (bottom-center pivot on CPU side)
    vec2  size;       // width, height (world units)
    float zSortKey;
    uint  slot;       // bindless index (tiles@binding0 or sprites@binding3)
    uint  flags;      // bit0: isSprite (1 = sprites@binding3, 0 = tiles@binding0)
    uint  _pad;
    uvec2 uvMin16;    // packed 0..65535 in 32-bit lanes
    uvec2 uvMax16;    // packed 0..65535 in 32-bit lanes
};

layout(location=0) out flat uint vSlot;
layout(location=1) out        vec2 vUV;
layout(location=2) out flat   uint vFlags;

layout(push_constant) uniform PC { mat4 VP; } pc;

// SSBO: set=0, binding=2
layout(std430, set=0, binding=2) readonly buffer Instances {
    Instance inst[];
};

const vec2 quad[4] = vec2[](
    vec2(0.0,0.0), vec2(1.0,0.0),
    vec2(0.0,1.0), vec2(1.0,1.0)
);

void main() {
    uint i = gl_InstanceIndex;
    vec2 q = quad[gl_VertexIndex];

    // Use ONE consistent convention:
    // If CPU gives center:
    //vec2 center = inst[i].worldPos; 
    // If CPU gives ground point instead, use:
    vec2 center = inst[i].worldPos + vec2(0.0, 0.5 * inst[i].size.y);

    vec2 pos = center + (q - vec2(0.5)) * inst[i].size;
    gl_Position = pc.VP * vec4(pos, 0.0, 1.0);

    // per-instance UVs with optional sprite V flip
    vec2 uvMin = vec2(inst[i].uvMin16) / 65535.0;
    vec2 uvMax = vec2(inst[i].uvMax16) / 65535.0;
    vec2 t = q;
    if ((inst[i].flags & 1u) != 0u) t.y = 1.0 - t.y; // flip only sprites
    vUV   = mix(uvMin, uvMax, t);

    vSlot  = inst[i].slot;
    vFlags = inst[i].flags;
}


#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in  flat uint vSlot;
layout(location=1) in        vec2 vUV;
layout(location=2) in  flat   uint vFlags;
layout(location=0) out        vec4 outColor;

// set=0, binding=0 : tiles (existing)
layout(set=0, binding=0) uniform sampler2D uTiles[];

// set=0, binding=3 : spritesheets (new)
layout(set=0, binding=3) uniform sampler2D uSprites[];

void main()
{
    bool isSprite = (vFlags & 1u) != 0u;

    vec4 base = isSprite
        ? texture(uSprites[nonuniformEXT(vSlot)], vUV)   // binding 3
        : texture(uTiles  [nonuniformEXT(vSlot)], vUV);  // binding 0

    if (base.a <= 0.001) discard;
    outColor = base;
}
