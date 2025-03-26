// ╔════════════════════════════════════════════════╗
// ║ 🚀 EVA ENGINE | 2D RENDERER - Quad SHADER		║
// ║        "Bending light, one pixel at a time."   ║
// ╚════════════════════════════════════════════════╝


#type vertex
#version 450 core

// Vertex attributes for each instance
layout(location = 0) in vec3 a_Position;       // Vertex position
layout(location = 1) in vec4 a_Color;          // Vertex color
layout(location = 2) in vec2 a_TexCoord;       // Texture coordinates
layout(location = 3) in float a_TexIndex;      // Texture index
layout(location = 4) in float a_TilingFactor;  // Tiling factor
layout(location = 5) in int a_EntityID;        // Entity ID

// Instance-specific transformation matrix
layout(location = 6) in mat4 a_TransformMatrix;  // Per-instance transform matrix

layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
};

struct VertexOutput
{
    vec4 Color;
    vec2 TexCoord;
    float TexIndex;
    float TilingFactor;
};

layout (location = 0) out VertexOutput Output;
layout (location = 4) out flat int v_EntityID;

void main()
{
    // Apply the transformation matrix for each instance
    vec4 transformedPosition = a_TransformMatrix * vec4(a_Position, 1.0);

    Output.Color = a_Color;
    Output.TexCoord = a_TexCoord;
    Output.TexIndex = a_TexIndex;
    Output.TilingFactor = a_TilingFactor;
    v_EntityID = a_EntityID;

    // Apply the view projection matrix
    gl_Position = u_ViewProjection * transformedPosition;
}

#type fragment
#version 450 core

layout(location = 0) out vec4 o_Color;
layout(location = 1) out int o_EntityID;

struct VertexOutput
{
	vec4 Color;
	vec2 TexCoord;
	float TexIndex;
	float TilingFactor;
};

layout (location = 0) in VertexOutput Input;
layout (location = 4) in flat int v_EntityID;

layout (binding = 0) uniform sampler2D u_Textures[32];

void main()
{
    // Use the TexIndex to sample from the texture array directly
    vec4 texColor = Input.Color * texture(u_Textures[int(Input.TexIndex)], Input.TexCoord * Input.TilingFactor);
    o_Color = texColor;
    o_EntityID = v_EntityID;
}