
//
// TODO tidy up!
// add ifdefs for float2/3/4, etc...
//

#define saturate(x) clamp((x), 0.0, 1.0)

//>HDR settings
#define HDRPower float3(1.30) //[0.00:8.00] //-Strangely lowering this makes the image brighter
#define radius2 0.87 //[0.00:8.00] //-Raising this seems to make the effect stronger and also brighter

float4 HDRPass( float4 colorInput, float2 Tex, sampler2D s0 )
{
    float3 c_center = texture2D(s0, Tex).rgb; //reuse SMAA center sample or lumasharpen center sample?
    //float3 c_center = colorInput.rgb; //or just the input?

    //float3 bloom_sum1 = float3(0.0, 0.0, 0.0); //don't initialize to 0 - use the first tex2D to do that
    //float3 bloom_sum2 = float3(0.0, 0.0, 0.0); //don't initialize to 0 - use the first tex2D to do that
    //Tex += float2(0, 0); // +0 ? .. oh riiiight - that will surely do something useful

    float radius1 = 0.793;
    float3 bloom_sum1 = texture2D(s0, Tex + float2(1.5, -1.5) * radius1).rgb;
    bloom_sum1 += texture2D(s0, Tex + float2(-1.5, -1.5) * radius1).rgb; //rearrange sample order to minimize ALU and maximize cache usage
    bloom_sum1 += texture2D(s0, Tex + float2(1.5, 1.5) * radius1).rgb;
    bloom_sum1 += texture2D(s0, Tex + float2(-1.5, 1.5) * radius1).rgb;

    bloom_sum1 += texture2D(s0, Tex + float2(0, -2.5) * radius1).rgb;
    bloom_sum1 += texture2D(s0, Tex + float2(0, 2.5) * radius1).rgb;
    bloom_sum1 += texture2D(s0, Tex + float2(-2.5, 0) * radius1).rgb;
    bloom_sum1 += texture2D(s0, Tex + float2(2.5, 0) * radius1).rgb;

    bloom_sum1 *= 0.005;

    float3 bloom_sum2 = texture2D(s0, Tex + float2(1.5, -1.5) * radius2).rgb;
    bloom_sum2 += texture2D(s0, Tex + float2(-1.5, -1.5) * radius2).rgb;
    bloom_sum2 += texture2D(s0, Tex + float2(1.5, 1.5) * radius2).rgb;
    bloom_sum2 += texture2D(s0, Tex + float2(-1.5, 1.5) * radius2).rgb;


    bloom_sum2 += texture2D(s0, Tex + float2(0, -2.5) * radius2).rgb;
    bloom_sum2 += texture2D(s0, Tex + float2(0, 2.5) * radius2).rgb;
    bloom_sum2 += texture2D(s0, Tex + float2(-2.5, 0) * radius2).rgb;
    bloom_sum2 += texture2D(s0, Tex + float2(2.5, 0) * radius2).rgb;

    bloom_sum2 *= 0.010;

    float dist = radius2 - radius1;

    float3 HDR = (c_center + (bloom_sum2 - bloom_sum1)) * dist;
    float3 blend = HDR + colorInput.rgb;

    colorInput.rgb = pow(abs(blend), HDRPower) + HDR; // pow - don't use fractions for HDRpower

//  colorInput.rgb = pow(abs(blend), abs(HDRPower)) + HDR; // pow - don't use fractions for HDRpower

    return saturate(colorInput);
}
