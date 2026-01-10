#type vertex
#version 450

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in uint a_TexIndex;

layout(set = 0, binding = 0) uniform CameraUBO
{
    mat4 u_ViewProjection;
} u_Camera;

layout(location = 0) out vec4 v_Color;
layout(location = 1) out vec2 v_UV;
layout(location = 2) flat out uint v_TexIndex;
layout(location = 3) out vec2 v_PosUI;

layout(push_constant) uniform UIParams
{
    float u_GlobalAlpha;
    float u_RoundRadiusPx;
    float u_RoundFeatherPx;
    float _pad0;
    vec4  u_ClipRectPx; // minX,minY,maxX,maxY
} u_UI;

void main()
{
    v_Color = a_Color;
    v_UV = a_TexCoord;
    v_TexIndex = a_TexIndex;

    // a_Position.xy is in UI pixel space already
    v_PosUI = a_Position.xy;

    gl_Position = u_Camera.u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 450

// If you use runtime array indexing in GLSL, enable this.
// (Some compilers need it for sampler arrays with non-constant index.)
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) in vec4 v_Color;
layout(location = 1) in vec2 v_UV;
layout(location = 2) flat in uint v_TexIndex;
layout(location = 3) in vec2 v_PosUI;

layout(location = 0) out vec4 o_Color;

layout(set = 1, binding = 0) uniform sampler2D u_Textures[];

layout(push_constant) uniform UIParams
{
    float u_GlobalAlpha;
    float u_RoundRadiusPx;
    float u_RoundFeatherPx;
    float _pad0;
    vec4  u_ClipRectPx; // minX,minY,maxX,maxY
} u_UI;

float sdRoundRect(vec2 p, vec2 rectCenter, vec2 rectHalfSize, float r)
{
    vec2 q = abs(p - rectCenter) - rectHalfSize + vec2(r);
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - r;
}

void main()
{
    // For R8 atlas: coverage in .r
    float coverage = texture(u_Textures[nonuniformEXT(v_TexIndex)], v_UV).r;

    // Build final color from vertex tint, with atlas coverage as alpha
    vec4 col = vec4(v_Color.rgb, v_Color.a * coverage);

    // Global alpha multiplier
    col.a *= u_UI.u_GlobalAlpha;

    // Clip rect (optional)
    if (u_UI.u_ClipRectPx.z > u_UI.u_ClipRectPx.x) // maxX > minX
    {
        if (v_PosUI.x < u_UI.u_ClipRectPx.x || v_PosUI.y < u_UI.u_ClipRectPx.y ||
            v_PosUI.x > u_UI.u_ClipRectPx.z || v_PosUI.y > u_UI.u_ClipRectPx.w)
        {
            discard;
        }
    }

    // Rounded corners (optional; usually you won’t use this for text, but harmless)
    if (u_UI.u_RoundRadiusPx > 0.0 && u_UI.u_ClipRectPx.z > u_UI.u_ClipRectPx.x)
    {
        vec2 minP = u_UI.u_ClipRectPx.xy;
        vec2 maxP = u_UI.u_ClipRectPx.zw;
        vec2 center = 0.5 * (minP + maxP);
        vec2 halfSize = 0.5 * (maxP - minP);

        float d = sdRoundRect(v_PosUI, center, halfSize, u_UI.u_RoundRadiusPx);

        float feather = max(u_UI.u_RoundFeatherPx, 0.0001);
        float alphaMask = 1.0 - smoothstep(0.0, feather, d);
        col.a *= alphaMask;
    }

    // Drop fully transparent pixels (helps avoid dark fringes if blending state is odd)
    if (col.a <= 0.001)
        discard;

    o_Color = col;
}
