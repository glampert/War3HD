
#include "FXAA.h"

uniform vec2 u_RcpScreenSize;
uniform sampler2D u_ColorRenderTarget;

void main()
{
    vec3 rtColor = FxaaPixelShader(gl_TexCoord[0].xy, u_ColorRenderTarget, u_RcpScreenSize);
    gl_FragColor = vec4(rtColor, 1.0);
}
