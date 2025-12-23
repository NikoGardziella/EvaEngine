#type vertex
#version 450 core

layout(location = 0) in vec3  a_Position;
layout(location = 1) in vec4  a_Color;
layout(location = 2) in vec2  a_TexCoord;
layout(location = 3) in uint  a_TexIndex; 
layout(location = 4) in float a_TilingFactor;

layout(std140, set = 0, binding = 0) uniform Camera
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


//****************************************************/
#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;

layout(location = 0) in vec4  v_Color;
layout(location = 1) in vec2  v_TexCoord;
layout(location = 2) in float v_TilingFactor;
layout(location = 3) in flat uint v_TexIndex;

layout(set = 1, binding = 1) uniform sampler2D u_Textures[32];

void main()
{
    uint idx = min(v_TexIndex, 31u);
    vec4 tex = texture(u_Textures[idx], v_TexCoord * v_TilingFactor);
    o_Color = v_Color * tex;
}

