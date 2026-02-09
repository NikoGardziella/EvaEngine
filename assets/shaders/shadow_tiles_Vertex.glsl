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
layout(location=1) out vec2 vUV;
layout(location=2) out flat uint vFlags;


layout(push_constant) uniform PC {
    mat4 VP_unsued;       // Offset 0 dont remove
    mat4 lightSpaceMatrix; // Offset 64
} pc;


layout(std430, set=0, binding=2) readonly buffer Instances { Instance inst[]; };

const vec2 quad[4] = vec2[](
    vec2(0.0,0.0), vec2(1.0,0.0),
    vec2(0.0,1.0), vec2(1.0,1.0)
);

void main()
{
    uint i = gl_InstanceIndex;
    vec2 q = quad[gl_VertexIndex];
    
    // 1. Calculate world position exactly like your main shader
    vec2 center = inst[i].worldPos + vec2(0.0, 0.5 * inst[i].size.y);
    vec2 local = (q - vec2(0.5)) * inst[i].size;
    float ang = inst[i].rotation;
    float c = cos(ang);
    float s = sin(ang);
    mat2 R = mat2(c, s, -s, c);
    vec2 pos = center + (R * local);

    // 2. Depth bias: Move shadow casters slightly toward the light 
    // to prevent Z-fighting with the ground.

    float spriteHeight = inst[i].size.y;

    // use q.y to connect the shadow to the base of the tile
    // spriteHeight increases the shadows size to match 3d
    //float shadowZ = q.y * spriteHeight; 


    float thickness = inst[i].size.x * 0.1; 
    // Offset the position slightly based on the vertex index or local X
    float shadowXOffset = (local.x > 0.0) ? thickness : -thickness;

   

    float heightPercent = q.y; 

    // If your sprite's worldPos is the BOTTOM center:
    vec2 basePos = inst[i].worldPos;
    // Calculate local offset but keep Y relative to the base (0.0)
    vec2 localPos = vec2((q.x - 0.5) * inst[i].size.x, q.y * inst[i].size.y);

    // Rotate and add to base
    vec2 finalPos = basePos + (R * localPos);

    //float shadowZ = heightPercent * inst[i].size.y;
    float shadowZ = (q.y * spriteHeight) - 0.1;

    // Subtracting the lean moves the TOP of the sprite, 
    // but because (q.y) is 0.0 at the base, lean is 0.0 at the base.
    // This "pins" the feet.
    gl_Position = pc.lightSpaceMatrix * vec4(finalPos, shadowZ, 1.0);

        // 3. Pass UVs for alpha clipping in the fragment shader
    vSlot = inst[i].slot;
    vFlags = inst[i].flags;
    vec2 uvMin = vec2(inst[i].uvMin16) / 65535.0;
    vec2 uvMax = vec2(inst[i].uvMax16) / 65535.0;
    
    // Apply Y-flip if the flag is set (matching your main shader logic)
    vec2 t = q;
    if ((inst[i].flags & 1u) != 0u) t.y = 1.0 - t.y;
    vUV = mix(uvMin, uvMax, t);
}

#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in flat uint vSlot;
layout(location=1) in vec2 vUV;
layout(location=2) in flat uint vFlags;
layout(location=0) out vec4 outColor;

layout(set=0, binding=0) uniform sampler2D uTiles[];

void main()
{
   uint i = vSlot;
    vec4 texColor = texture(uTiles[nonuniformEXT(vSlot)], vUV);
    if (texColor.a < 0.5)
    {
        discard; 
    }
}