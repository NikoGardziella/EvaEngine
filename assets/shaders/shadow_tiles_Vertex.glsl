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

layout(location=0) out flat uint vSlot;
layout(location=1) out vec2 vUV;
layout(location=2) out flat int vFaceId;
layout(location=3) out float vFootY;
layout(location=4) out vec2 vEdgeUV;

layout(push_constant) uniform PC {
    vec4 lightDirection;
    mat4 lightSpaceMatrix;
} pc;

layout(std430, set=0, binding=2) readonly buffer Instances { Instance inst[]; };

const vec3 boxVerts[36] = vec3[](
    vec3(-0.5, 0.0, 0.5), vec3( 0.5, 0.0, 0.5), vec3( 0.5, 1.0, 0.5),
    vec3( 0.5, 1.0, 0.5), vec3(-0.5, 1.0, 0.5), vec3(-0.5, 0.0, 0.5),
    vec3( 0.5, 0.0,-0.5), vec3(-0.5, 0.0,-0.5), vec3(-0.5, 1.0,-0.5),
    vec3(-0.5, 1.0,-0.5), vec3( 0.5, 1.0,-0.5), vec3( 0.5, 0.0,-0.5),
    vec3(-0.5, 0.0,-0.5), vec3(-0.5, 0.0, 0.5), vec3(-0.5, 1.0, 0.5),
    vec3(-0.5, 1.0, 0.5), vec3(-0.5, 1.0,-0.5), vec3(-0.5, 0.0,-0.5),
    vec3( 0.5, 0.0, 0.5), vec3( 0.5, 0.0,-0.5), vec3( 0.5, 1.0,-0.5),
    vec3( 0.5, 1.0,-0.5), vec3( 0.5, 1.0, 0.5), vec3( 0.5, 0.0, 0.5),
    vec3(-0.5, 1.0, 0.5), vec3( 0.5, 1.0, 0.5), vec3( 0.5, 1.0,-0.5),
    vec3( 0.5, 1.0,-0.5), vec3(-0.5, 1.0,-0.5), vec3(-0.5, 1.0, 0.5),
    vec3(-0.5, 0.0,-0.5), vec3( 0.5, 0.0,-0.5), vec3( 0.5, 0.0, 0.5),
    vec3( 0.5, 0.0, 0.5), vec3(-0.5, 0.0, 0.5), vec3(-0.5, 0.0,-0.5)
);

// ============================================================
// Offset
// ============================================================
float directionToAngle(uint direction)
{
    // 0=N, 1=S, 2=E, 3=W, 4=Center, 5=unknonw, 
    switch (direction)
    {
        case 0: return radians(30.0);
        case 1: return radians(30.0);
        case 2: return radians(150.0);
        case 3: return radians(150.0);
        case 4: return radians(0.0);
        case 5: return radians(0.0);
      
    }
}

vec2 directionToOffset(uint direction)
{
    switch (direction)
    {
        case 0: return vec2( 0.25,   0.5);   // N
        case 1: return vec2( 0.0,   0.75);   // S
        case 2: return vec2( -0.25,   0.5);   // E
        case 3: return vec2( 0.40,   0.65);   // W
        case 4: return vec2( 0.0,   0.0);   // Center
        case 5: return vec2( 0.0,   0.0);   // unknonw
       
    }
}



mat2 buildRotation(float ang)
{
    return mat2(cos(ang), sin(ang), -sin(ang), cos(ang));
}


struct OpaqueBox {
    float w;
    float h;
    float depth;
    float centerX;
    float bottomY;
};

OpaqueBox computeOpaqueBox(vec2 oMin, vec2 oMax, float tileW, float tileH)
{
    OpaqueBox b;
    b.w = tileW * (oMax.x - oMin.x);
    b.h = tileH * (oMax.y - oMin.y);
    b.depth = b.w * 0.5;
    b.centerX = tileW * ((oMin.x + oMax.x) * 0.5 - 0.5);
    b.bottomY = tileH * oMin.y;
    return b;
}


vec3 boxToWorld(vec3 v, OpaqueBox box, mat2 R, vec2 worldAnchor, vec2 offset, bool isRoof)
{


    vec3 localPos;

    if (isRoof)
    {
        // FLAT ROOF LOGIC
        // We take the vertical height of the tile (box.h) and lay it down 
        // along the depth axis (Y in localPos calculation).
        localPos = vec3(
            v.x * box.w - 0.5,
            v.y * box.h + 0.5,
            box.bottomY + box.h
        );
    }
    else
    {
        // NORMAL WALL LOGIC
        localPos = vec3(
            v.x * box.w,
            v.z * box.depth,
            box.bottomY + v.y * box.h
        );
    }

    vec2 rotatedXY = R * localPos.xy;
    return vec3(worldAnchor + rotatedXY + offset, localPos.z);
}

void computeFaceUVs(int faceIdx, vec3 v, uint flags, vec2 opaqueUVMin, vec2 opaqueUVMax,  out vec2 uv, out vec2 edgeUV)
{
    if (faceIdx < 2)
    {
        // Front / back
        vec2 t = vec2(v.x + 0.5, v.y);
        if ((flags & 1u) != 0u) t.y = 1.0 - t.y;
        uv = mix(opaqueUVMin, opaqueUVMax, t);
        edgeUV = vec2(0.0);
    }
    else if (faceIdx == 2)
    {
        // Left: leftmost column
        uv = vec2(0.0);
        edgeUV = mix(opaqueUVMin, opaqueUVMax, vec2(0.0, v.y));
    }
    else if (faceIdx == 3)
    {
        // Right: rightmost column
        uv = vec2(0.0);
        edgeUV = mix(opaqueUVMin, opaqueUVMax, vec2(1.0, v.y));
    }
    else if (faceIdx == 4)   // TOP face of the box
    {
        edgeUV = mix(opaqueUVMin, opaqueUVMax, vec2(v.x + 0.5, 0.5));
    }
    else                     // BOTTOM face of the box
    {
        edgeUV = mix(opaqueUVMin, opaqueUVMax, vec2(v.x + 0.5, 0.5));
    }
}


void main()
{
    uint i = gl_InstanceIndex;
    int vIdx = int(gl_VertexIndex % 36);
    int faceIdx = vIdx / 6;
    vec3 v = boxVerts[vIdx];
    bool isRoof = (inst[i].flags & 2u) != 0u;

    // Opaque region
    vec2 oMin = vec2(inst[i].opaqueMin16) / 65535.0;
    vec2 oMax = vec2(inst[i].opaqueMax16) / 65535.0;
    OpaqueBox box = computeOpaqueBox(oMin, oMax, inst[i].size.x, inst[i].size.y);

    if (isRoof)
    {
        // This prevents the shadow from looking like a thick solid block
        box.depth = 0.05; // Very thin
    }
    // Rotation from direction
    float ang = directionToAngle(inst[i].direction);
    mat2 R = buildRotation(ang);

    // World position
    vec2 offset = directionToOffset(inst[i].direction);
    vec3 worldPos = boxToWorld(v, box, R, inst[i].worldPos, offset, isRoof);



    gl_Position = pc.lightSpaceMatrix * vec4(worldPos, 1.0);

    // Opaque UV range
    vec2 uvMin = vec2(inst[i].uvMin16) / 65535.0;
    vec2 uvMax = vec2(inst[i].uvMax16) / 65535.0;
    vec2 opaqueUVMin = mix(uvMin, uvMax, oMin);
    vec2 opaqueUVMax = mix(uvMin, uvMax, oMax);

    // Per-face UVs
    computeFaceUVs(faceIdx, v, inst[i].flags, opaqueUVMin, opaqueUVMax, vUV, vEdgeUV);

    // Outputs
    vFaceId = faceIdx;
    vSlot = inst[i].slot;
    vFootY = inst[i].worldPos.y;
}



#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in flat uint vSlot;
layout(location=1) in vec2 vUV;
layout(location=2) in flat int vFaceId;
layout(location=3) in float vFootY;
layout(location=4) in vec2 vEdgeUV;  
layout(location=0) out vec2 outData;

layout(set=0, binding=0) uniform sampler2D uTiles[];

void main()
{
    if (vFaceId < 2)
    {
        // Front/back: normal alpha test
        float alpha = texture(uTiles[nonuniformEXT(vSlot)], vUV).a;
        if (alpha < 0.5) discard;
    }
    else 
    {
        // Sides/top/bottom: sample the nearest front-face edge pixel
        float alpha = texture(uTiles[nonuniformEXT(vSlot)], vEdgeUV).a;
        if (alpha < 0.5) discard;
    }

    outData = vec2(gl_FragCoord.z, vFootY);
}