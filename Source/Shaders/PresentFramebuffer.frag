
uniform sampler2D u_ColorRenderTarget;

void main()
{
    vec4 rtColor = texture2D(u_ColorRenderTarget, gl_TexCoord[0].xy);
    gl_FragColor = rtColor;
}
