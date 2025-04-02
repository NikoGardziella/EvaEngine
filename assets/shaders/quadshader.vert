#version 450 core
#type vertex

// Vertex attributes for each instance
layout(location = 0) in vec3 a_Position;       // Vertex position
layout(location = 1) in vec4 a_Color;          // Vertex color
layout(location = 2) in vec2 a_TexCoord;       // Texture coordinates
layout(location = 3) in float a_TexIndex;      // Texture index
layout(location = 4) in float a_TilingFactor;  // Tiling factor
layout(location = 5) in int a_EntityID;        // Entity ID

// Instance-specific transformation matrix
//layout(location = 6) in vec4 a_InstanceTransform0;
//layout(location = 7) in vec4 a_InstanceTransform1;
//layout(location = 8) in vec4 a_InstanceTransform2;
//layout(location = 9) in vec4 a_InstanceTransform3;



layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
};

struct VertexOutput
{
    vec4 Color;
    vec2 TexCoord;
    float TilingFactor;
};

layout (location = 0) out VertexOutput Output;
layout (location = 3) out flat float v_TexIndex;
layout (location = 4) out flat int v_EntityID;

void main()
{
	//mat4 a_InstanceTransform = mat4(
	//	a_InstanceTransform0,
	//	a_InstanceTransform1,
	//	a_InstanceTransform2,
	//	a_InstanceTransform3
	//	);
    // Apply the transformation matrix for each instance
    //vec4 transformedPosition = a_InstanceTransform * vec4(a_Position, 1.0);



    Output.Color = a_Color;
    Output.TexCoord = a_TexCoord;
    Output.TilingFactor = a_TilingFactor;
    v_TexIndex = a_TexIndex;
    v_EntityID = a_EntityID;

    // Apply the view projection matrix

	//gl_Position = u_ViewProjection * transformedPosition;
	
	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
	//gl_Position = vec4(a_Position, 1.0);
	
	
}
