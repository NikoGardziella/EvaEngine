// Flat Color Shader

#type vertex
#version 450 core
			
layout(location = 0) in vec3 a_position; // Vertex position (in object space)

uniform mat4 u_viewProjection; // View-projection matrix (camera transform)
uniform mat4 u_transform;      // Object-to-world transform matrix


void main()
{
	// Transform the position to clip space
	gl_Position = u_viewProjection * u_transform * vec4(a_position, 1.0);
}


#type fragment
#version 450 core

precision mediump float; // Ensures cross-platform compatibility

layout(location = 0) out vec4 color; // Output color of the fragment

uniform vec4 u_color;

void main()
{

	color = u_color;
}