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
layout(location=3) out float vWorldY;
layout(location=4) out flat int vFaceType;

layout(push_constant) uniform PC {
    vec4 lightDirection;       
    mat4 lightSpaceMatrix; 
} pc;

layout(std430, set=0, binding=2) readonly buffer Instances { Instance inst[]; };

// 6 vertices for a single flat quad (Front face silhouette)
const vec3 unitQuad[6] = vec3[](
    vec3(-0.5, 0.0, 0.0), // Bottom Left
    vec3( 0.5, 0.0, 0.0), // Bottom Right
    vec3( 0.5, 1.0, 0.0), // Top Right
    
    vec3( 0.5, 1.0, 0.0), // Top Right
    vec3(-0.5, 1.0, 0.0), // Top Left
    vec3(-0.5, 0.0, 0.0)  // Bottom Left
);

void main()
{
    uint i = gl_InstanceIndex;
    int vIdx = int(gl_VertexIndex % 6); 
    vec3 quadPos = unitQuad[vIdx]; // FIXED: No more +6 offset

    // 1. Calculate Local Position
    // localPos.z is the height of the sprite
    // Scale the width (x) slightly to bridge the gap between tiles
    float overlapFactor = 1.09; 
    vec3 localPos = vec3(
        quadPos.x * inst[i].size.x * overlapFactor, 
        0.0, 
        quadPos.y * inst[i].size.y
    );

    // 2. THE LEAN-BACK (Standard 2.5D Trick)
    // We tilt the sprite slightly back on the Y axis based on its height.
    // This ensures it has a "footprint" on the floor even if the light is 90 deg.
    localPos.y += localPos.z * 0.15; 

    // 3. Rotation (Horizontal Plane)
    float ang = radians(35.0); 
    mat2 R = mat2(cos(ang), sin(ang), -sin(ang), cos(ang));
    vec2 rotatedXY = R * localPos.xy;

    // 4. Construct World Position
    vec3 worldPos = vec3(inst[i].worldPos + rotatedXY, localPos.z);
    worldPos.y += 0.30;
    worldPos.x += 0.25;

    // 5. Outputs
    vSlot = inst[i].slot;
    vFlags = inst[i].flags;
    vWorldY = worldPos.y;
    vFaceType = 0; // Silhouette mode
    
    gl_Position = pc.lightSpaceMatrix * vec4(worldPos, 1.0);

    // 6. UVs
    vec2 t = vec2(quadPos.x + 0.5, quadPos.y);
    if ((inst[i].flags & 1u) != 0u) t.y = 1.0 - t.y;

    vec2 uvMin = vec2(inst[i].uvMin16) / 65535.0;
    vec2 uvMax = vec2(inst[i].uvMax16) / 65535.0;
    vUV = mix(uvMin, uvMax, t);
}

#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in flat uint vSlot;
layout(location=1) in vec2 vUV;
layout(location=2) in flat uint vFlags;
layout(location=3) in float vWorldY;
layout(location=4) flat in int vFaceType;

layout(location=0) out vec2 outData; 
layout(set=0, binding=0) uniform sampler2D uTiles[];

void main()
{
    vec4 texColor = texture(uTiles[nonuniformEXT(vSlot)], vUV);

    // Alpha discard is essential for the silhouette look
    if (texColor.a < 0.5) discard;

    outData = vec2(gl_FragCoord.z, vWorldY);
}