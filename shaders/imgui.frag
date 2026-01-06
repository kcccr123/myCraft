#if defined(GL_ES)
    #version 300 es
    precision mediump float;
#else
    #version 130
#endif

uniform sampler2D Texture;

in vec2 Frag_UV;
in vec4 Frag_Color;

#if __VERSION__ >= 300
    layout (location = 0) out vec4 Out_Color;
#else
    out vec4 Out_Color;
#endif

void main()
{
#if __VERSION__ >= 130
    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);
#else
    gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV.st);
#endif
}
