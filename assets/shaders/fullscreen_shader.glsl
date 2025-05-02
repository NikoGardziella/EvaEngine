#type vertex
#version 450 core

layout(location = 0) out vec2 v_TexCoord;

void main()
{
    vec2 positions[3] = vec2[](
        vec2(-1.0, -3.0),
        vec2(3.0, 1.0),
        vec2(-1.0, 1.0)
    );

    vec2 texCoords[3] = vec2[](
        vec2(0.0, -1.0),
        vec2(2.0, 1.0),
        vec2(0.0, 1.0)
    );

    v_TexCoord = texCoords[gl_VertexIndex];
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);

}



//****************************************************/

#type fragment
#version 450 core

layout(set = 0, binding = 0) uniform sampler2D u_Texture;

layout(location = 0) in vec2 v_TexCoord;
layout(location = 0) out vec4 o_Color;

void main()
{
   vec2 flippedCoord = vec2(v_TexCoord.x, 1.0 - v_TexCoord.y);
    o_Color = texture(u_Texture, flippedCoord);
    //o_Color = texture(u_Texture, v_TexCoord);
}

