
#include "FXAA.h"
#include "PostProcessingFilters.h"

// NOTE: Matching constants in ShaderProgram.hpp
const int kPostProcessFXAA  = 1 << 1;
const int kPostProcessHDR   = 1 << 2;
const int kPostProcessBloom = 1 << 3;
const int kPostProcessNoise = 1 << 4;

uniform int u_PostProcessFlags;
uniform vec2 u_RcpScreenSize;
uniform sampler2D u_ColorRenderTarget;

void main()
{
    vec2 uvs = gl_TexCoord[0].xy;
    vec3 rtColor;

    // Apply FXAA anti-aliasing:
    if ((u_PostProcessFlags & kPostProcessFXAA) != 0)
    {
        rtColor = FxaaPixelShader(uvs, u_ColorRenderTarget, u_RcpScreenSize);
    }
    else
    {
        rtColor = texture2D(u_ColorRenderTarget, uvs).rgb;
    }

    // Apply other post-processing image filters:
    if ((u_PostProcessFlags & kPostProcessHDR) != 0)
    {
        rtColor = HDRPass(uvs, u_ColorRenderTarget, rtColor);
    }
    if ((u_PostProcessFlags & kPostProcessBloom) != 0)
    {
        rtColor = BloomPass(uvs, u_ColorRenderTarget, rtColor);
    }
    if ((u_PostProcessFlags & kPostProcessNoise) != 0)
    {
        rtColor = NoisePass(uvs, u_ColorRenderTarget, rtColor);
    }

    gl_FragColor = vec4(rtColor, 1.0);
}
