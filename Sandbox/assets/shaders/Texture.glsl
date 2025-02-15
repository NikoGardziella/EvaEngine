#type vertex
#version 450 core
			
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec4 a_color;
layout(location = 2) in vec2 a_texCoord;    
layout(location = 3) in float a_texIndex;    
layout(location = 4) in float a_tiling;    

uniform mat4 u_viewProjection; 

out vec2 v_texCoord; 
out vec4 v_color; 
out float v_texIndex;
out float v_tiling;

void main()
{
	v_texCoord = a_texCoord;
	v_color = a_color;
	v_texIndex = a_texIndex;
	v_tiling = a_tiling;
	gl_Position = u_viewProjection * vec4(a_position, 1.0);
}


#type fragment
#version 450 core

layout(location = 0) out vec4 color; // Output color of the fragment

in vec2 v_texCoord;
in vec4 v_color;
in float v_texIndex;
in float v_tiling;

uniform sampler2D u_textures[32];


void main()
{
	color = texture(u_textures[int(v_texIndex)], v_texCoord * v_tiling) * v_color;
}