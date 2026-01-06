#if defined(GL_ES)
    #version 300 es
    precision highp float;
#else
    #version 130
#endif

uniform mat4 ProjMtx;

#if __VERSION__ >= 300
    layout (location = 0) in vec2 Position;
    layout (location = 1) in vec2 UV;
    layout (location = 2) in vec4 Color;
#else
    in vec2 Position;
    in vec2 UV;
    in vec4 Color;
#endif

out vec2 Frag_UV;
out vec4 Frag_Color;

void main()
{
    Frag_UV = UV;
    Frag_Color = Color;
    gl_Position = ProjMtx * vec4(Position.xy, 0, 1);
}
