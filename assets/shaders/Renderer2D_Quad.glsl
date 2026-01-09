#type vertex
#version 450 core

// Per-vertex inputs (your quad vertices already in world space)
layout(location = 0) in vec3  a_Position;
layout(location = 1) in vec4  a_Color;
layout(location = 2) in vec2  a_TexCoord;
layout(location = 3) in uint  a_TexIndex;      // was float -> use uint
layout(location = 4) in float a_TilingFactor;
layout(location = 5) in int   a_EntityID;

layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
};

layout(location = 0) out vec4  v_Color;
layout(location = 1) out vec2  v_TexCoord;
layout(location = 2) out float v_TilingFactor;
layout(location = 3) out flat uint v_TexIndex;

void main()
{
    v_Color        = a_Color;
    v_TexCoord     = a_TexCoord;
    v_TilingFactor = a_TilingFactor;
    v_TexIndex     = a_TexIndex;

    gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int  o_EntityID;

layout(location = 0) in vec4  v_Color;
layout(location = 1) in vec2  v_TexCoord;
layout(location = 2) in float v_TilingFactor;
layout(location = 3) in flat uint v_TexIndex;


layout(set = 1, binding = 1) uniform sampler2D u_Textures[32];

void main()
{
    // Clamp to avoid OOB if something goes wrong on CPU side
    uint idx = min(v_TexIndex, 31u);

    vec4 texSample = texture(u_Textures[idx], v_TexCoord * v_TilingFactor);
    vec4 texColor  = v_Color * texSample;

    if (texColor.a <= 0.0)
        discard;

    o_Color    = texColor;
}
