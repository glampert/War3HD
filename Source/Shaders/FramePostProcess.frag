
// WORK IN PROGRESS

#include "FXAA.h"
#include "HDR.h"

// GTA-V shader reference:
// https://github.com/crosire/reshade-shaders/tree/master/ReShade

/// -----------------------------------------------

#define BloomThreshold 20.25 //[0.00:50.00] //-Threshold for what is a bright light (that causes bloom) and what isn't.
#define BloomPower 1.446 //[0.000:8.000] //-Strength of the bloom
#define BloomWidth 0.0142 //[0.0000:1.0000] //-Width of the bloom
#define RFX_PixelSize 1.0

// Makes bright lights bleed their light into their surroundings (relatively high performance cost)
float4 BloomPass( float4 ColorInput2, float2 Tex, sampler2D s0  )
{
    float3 BlurColor2 = float3(0.0);
    float3 Blurtemp = float3(0.0);
    //float MaxDistance = sqrt(8*BloomWidth);
    float MaxDistance = 8.0*BloomWidth; //removed sqrt
    float CurDistance = 0.0;

    //float Samplecount = 0;
    float Samplecount = 25.0;

    float2 blurtempvalue = float2(Tex * RFX_PixelSize * BloomWidth);

    //float distancetemp = 1.0 - ((MaxDistance - CurDistance) / MaxDistance);

    float2 BloomSample = float2(2.5,-2.5);
    float2 BloomSampleValue;// = BloomSample;

    for(BloomSample.x = 2.5; BloomSample.x > -2.0; BloomSample.x = BloomSample.x - 1.0) // runs 5 times
    {
        BloomSampleValue.x = BloomSample.x * blurtempvalue.x;
        float2 distancetemp = float2(BloomSample.x * BloomSample.x * BloomWidth);

        for(BloomSample.y = (- 2.5); BloomSample.y < 2.0; BloomSample.y = BloomSample.y + 1.0) // runs 5 ( * 5) times
        {
            distancetemp.y = BloomSample.y * BloomSample.y;
            //CurDistance = sqrt(dot(BloomSample,BloomSample)*BloomWidth); //dot() attempt - same result , same speed. //move x part up ?
            //CurDistance = sqrt( (distancetemp.y * BloomWidth) + distancetemp.x); //dot() attempt - same result , same speed. //move x part up ?
            CurDistance = (distancetemp.y * BloomWidth) + distancetemp.x; //removed sqrt

            //Blurtemp.rgb = texture2D(s0, float2(Tex + (BloomSample*blurtempvalue))); //same result - same speed.
            BloomSampleValue.y = BloomSample.y * blurtempvalue.y;
            Blurtemp.rgb = texture2D(s0, float2(Tex + BloomSampleValue)).rgb; //same result - same speed.

            //BlurColor2.rgb += mix(Blurtemp.rgb,ColorInput2.rgb, 1 - ((MaxDistance - CurDistance)/MaxDistance)); //convert float4 to float3 and check if it's possible to use a MAD
            //BlurColor2.rgb += mix(Blurtemp.rgb,ColorInput2.rgb, 1.0 - ((MaxDistance - CurDistance) / MaxDistance)); //convert float4 to float3 and check if it's possible to use a MAD
            BlurColor2.rgb += mix(Blurtemp.rgb,ColorInput2.rgb, sqrt(CurDistance / MaxDistance)); //reduced number of sqrts needed


            //Samplecount = Samplecount + 1; //take out of loop and replace with constant if it helps (check with compiler)
        }
    }
    BlurColor2.rgb = (BlurColor2.rgb / (Samplecount - (BloomPower - BloomThreshold*5))); //check if using MAD
    float Bloomamount = dot(ColorInput2.rgb,float3(0.299f, 0.587f, 0.114f)) ; //try BT 709
    float3 BlurColor = BlurColor2.rgb * (BloomPower + 4.0); //check if calculated offline and combine with line 24 (the blurcolor2 calculation)

    ColorInput2.rgb = mix(ColorInput2.rgb,BlurColor.rgb, Bloomamount);

    return saturate(ColorInput2);
}

#define Explosion_Radius 2.0 //[0.2:100.0] //-Amount of effect you want.

// Scatters the pixels, making the image look fuzzy.
float4 ExplosionPass( float4 colorInput, float2 tex, sampler2D s0 )
{
    // -- pseudo random number generator --
    float seed = dot(tex, float2(12.9898, 78.233));
    float2 sine_cosine;
    sine_cosine.x = sin(seed);
    sine_cosine.y = cos(seed);
    sine_cosine = sine_cosine * 43758.5453 + tex;
    float2 noise = fract(sine_cosine);

    tex = float2(-Explosion_Radius * RFX_PixelSize) + tex; //Slightly faster this way because it can be calculated while we calculate noise.

    colorInput.rgb = texture2D(s0, (2.0 * Explosion_Radius * RFX_PixelSize) * noise + tex).rgb;

    return colorInput;
}

/// -----------------------------------------------

uniform sampler2D u_ColorRenderTarget;

//FIXME temporary!
const vec2 c_rcpFrame = vec2(1.0/1440.0, 1.0/900.0);

void main()
{
    // pixelated filter
//    const float scale = 6;
//    vec4 source = texture2D(u_ColorRenderTarget, gl_TexCoord[0].xy);
//    gl_FragColor.xyz = round(source.xyz * scale) / scale;

    float4 color;
    float2 uvs = gl_TexCoord[0].xy;

    color.rgb = FxaaPixelShader(uvs, u_ColorRenderTarget, c_rcpFrame);
    color.a = 1.0;

    color = HDRPass(color, uvs, u_ColorRenderTarget);
    color = BloomPass(color, uvs, u_ColorRenderTarget);
//    color = ExplosionPass(color, uvs, u_ColorRenderTarget);

    //gl_FragColor = texture2D(u_ColorRenderTarget, gl_TexCoord[0].xy) * vec4(0.4, 1.0, 0.4, 1.0);

    //gl_FragColor = gl_TexCoord[0]; // vec4
    //gl_FragColor = vec4(v_Normal, 1.0);
    //gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);

    gl_FragColor = color;
}
