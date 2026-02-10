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
layout(location = 3) out float vWorldY;

layout(push_constant) uniform PC {
    vec4 lightDirection;       
    mat4 lightSpaceMatrix; 
} pc;

layout(std430, set=0, binding=2) readonly buffer Instances { Instance inst[]; };

const vec3 unitCube[36] = vec3[](
    // Back face
    vec3(-0.5, 0.0, -0.5), vec3( 0.5, 0.0, -0.5), vec3( 0.5, 1.0, -0.5),
    vec3( 0.5, 1.0, -0.5), vec3(-0.5, 1.0, -0.5), vec3(-0.5, 0.0, -0.5),
    // Front face
    vec3(-0.5, 0.0,  0.5), vec3( 0.5, 0.0,  0.5), vec3( 0.5, 1.0,  0.5),
    vec3( 0.5, 1.0,  0.5), vec3(-0.5, 1.0,  0.5), vec3(-0.5, 0.0,  0.5),
    // Left face
    vec3(-0.5, 1.0,  0.5), vec3(-0.5, 1.0, -0.5), vec3(-0.5, 0.0, -0.5),
    vec3(-0.5, 0.0, -0.5), vec3(-0.5, 0.0,  0.5), vec3(-0.5, 1.0,  0.5),
    // Right face
    vec3( 0.5, 1.0,  0.5), vec3( 0.5, 1.0, -0.5), vec3( 0.5, 0.0, -0.5),
    vec3( 0.5, 0.0, -0.5), vec3( 0.5, 0.0,  0.5), vec3( 0.5, 1.0,  0.5),
    // Bottom face
    vec3(-0.5, 0.0, -0.5), vec3( 0.5, 0.0, -0.5), vec3( 0.5, 0.0,  0.5),
    vec3( 0.5, 0.0,  0.5), vec3(-0.5, 0.0,  0.5), vec3(-0.5, 0.0, -0.5),
    // Top face
    vec3(-0.5, 1.0, -0.5), vec3( 0.5, 1.0, -0.5), vec3( 0.5, 1.0,  0.5),
    vec3( 0.5, 1.0,  0.5), vec3(-0.5, 1.0,  0.5), vec3(-0.5, 1.0, -0.5)
);
void main()
{
    uint i = gl_InstanceIndex;
    int vIdx = int(gl_VertexIndex % 36);
    vec3 cubePos = unitCube[vIdx];

    //float thickness = inst[i].size.x * 0.5; 
    float thickness = 0.01;


    // cube-Y (0 to 1) is Height (world-Z)
    // cube-Z is Thickness (world-Y)
    vec3 localPos = vec3(
        cubePos.x * inst[i].size.x, 
        cubePos.z * thickness,      
        cubePos.y * inst[i].size.y   
    );

    float ang = inst[i].rotation;
    float c = cos(ang);
    float s = sin(ang);
    mat2 R = mat2(c, s, -s, c);
    
    vec2 rotatedXY = R * localPos.xy;

    // PUSH-BACK: Move the shadow caster slightly into the screen (Y)
    // and slightly below the floor (Z) so it never clips the receiver.
    float pushBackY = 0.25; 

    vec3 worldPos = vec3(inst[i].worldPos + rotatedXY, localPos.z);
    worldPos.y += pushBackY; 
    

    float tileTag = 1000000.0f;
    vWorldY = worldPos.y + tileTag;

    gl_Position = pc.lightSpaceMatrix * vec4(worldPos, 1.0);

    vSlot = inst[i].slot;
    vFlags = inst[i].flags;
    vec2 uvMin = vec2(inst[i].uvMin16) / 65535.0;
    vec2 uvMax = vec2(inst[i].uvMax16) / 65535.0;
    
    vec2 t = vec2(cubePos.x + 0.5, cubePos.y);
    if ((inst[i].flags & 1u) != 0u) t.y = 1.0 - t.y;
    vUV = mix(uvMin, uvMax, t);
}


#type fragment
#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in flat uint vSlot;
layout(location=1) in vec2 vUV;
layout(location=2) in flat uint vFlags;
layout(location=3) in float vWorldY;

// CHANGE: This must be location 0 to match your RenderPass setup
layout(location=0) out vec2 outData; 

layout(set=0, binding=0) uniform sampler2D uTiles[];

// Inside your Shadow Fragment Shader (Tile version)
void main()
{
    vec4 texColor = texture(uTiles[nonuniformEXT(vSlot)], vUV);
    if (texColor.a < 0.5) discard;

    // Discard the bottom 2% of the texture.
    // This prevents the "feet" from creating acne on the tile they stand on.
    if (vUV.y > 0.98) discard; 

    outData = vec2(gl_FragCoord.z, vWorldY);
}