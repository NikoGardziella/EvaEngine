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

layout(location = 0) out vec4 color;
layout(location = 1) out int color2;

in vec2 v_texCoord;
in vec4 v_color;
in float v_texIndex;
in float v_tiling;

uniform sampler2D u_textures[32];

void main()
{
    vec4 sampledColor;
    
    int index = int(v_texIndex);
    
    switch (index)
    {
        // using an array of samplers indexed by a non-constant expression (like v_texIndex) is not guaranteed
        //to work across all GPUs, especially on AMD and mobile devices.

        case 0:  sampledColor = texture(u_textures[0],  v_texCoord * v_tiling); break;
        case 1:  sampledColor = texture(u_textures[1],  v_texCoord * v_tiling); break;
        case 2:  sampledColor = texture(u_textures[2],  v_texCoord * v_tiling); break;
        case 3:  sampledColor = texture(u_textures[3],  v_texCoord * v_tiling); break;
        case 4:  sampledColor = texture(u_textures[4],  v_texCoord * v_tiling); break;
        case 5:  sampledColor = texture(u_textures[5],  v_texCoord * v_tiling); break;
        case 6:  sampledColor = texture(u_textures[6],  v_texCoord * v_tiling); break;
        case 7:  sampledColor = texture(u_textures[7],  v_texCoord * v_tiling); break;
        case 8:  sampledColor = texture(u_textures[8],  v_texCoord * v_tiling); break;
        case 9:  sampledColor = texture(u_textures[9],  v_texCoord * v_tiling); break;
        case 10: sampledColor = texture(u_textures[10], v_texCoord * v_tiling); break;
        case 11: sampledColor = texture(u_textures[11], v_texCoord * v_tiling); break;
        case 12: sampledColor = texture(u_textures[12], v_texCoord * v_tiling); break;
        case 13: sampledColor = texture(u_textures[13], v_texCoord * v_tiling); break;
        case 14: sampledColor = texture(u_textures[14], v_texCoord * v_tiling); break;
        case 15: sampledColor = texture(u_textures[15], v_texCoord * v_tiling); break;
        case 16: sampledColor = texture(u_textures[16], v_texCoord * v_tiling); break;
        case 17: sampledColor = texture(u_textures[17], v_texCoord * v_tiling); break;
        case 18: sampledColor = texture(u_textures[18], v_texCoord * v_tiling); break;
        case 19: sampledColor = texture(u_textures[19], v_texCoord * v_tiling); break;
        case 20: sampledColor = texture(u_textures[20], v_texCoord * v_tiling); break;
        case 21: sampledColor = texture(u_textures[21], v_texCoord * v_tiling); break;
        case 22: sampledColor = texture(u_textures[22], v_texCoord * v_tiling); break;
        case 23: sampledColor = texture(u_textures[23], v_texCoord * v_tiling); break;
        case 24: sampledColor = texture(u_textures[24], v_texCoord * v_tiling); break;
        case 25: sampledColor = texture(u_textures[25], v_texCoord * v_tiling); break;
        case 26: sampledColor = texture(u_textures[26], v_texCoord * v_tiling); break;
        case 27: sampledColor = texture(u_textures[27], v_texCoord * v_tiling); break;
        case 28: sampledColor = texture(u_textures[28], v_texCoord * v_tiling); break;
        case 29: sampledColor = texture(u_textures[29], v_texCoord * v_tiling); break;
        case 30: sampledColor = texture(u_textures[30], v_texCoord * v_tiling); break;
        case 31: sampledColor = texture(u_textures[31], v_texCoord * v_tiling); break;
        default: sampledColor = vec4(1.0, 0.0, 1.0, 1.0); // Magenta color for debugging (invalid texture index)
    }

    color = sampledColor * v_color;
    color2 = 50;
}
