
// Functions taken from the GTA-V reshade project:
// https://github.com/crosire/reshade-shaders/tree/master/ReShade

vec3 HDRPass(vec2 texCoords, sampler2D s0, vec3 colorInput)
{
    // HDR settings:
    const vec3 kHDRPower = vec3(1.3); // Lowering this makes the image brighter.
    const float kRadius1 = 0.793;
    const float kRadius2 = 0.87; // Raising this make the effect stronger and also brighter.
    const float kDist = kRadius2 - kRadius1;

    vec3 bloomSum1;
    bloomSum1  = texture2D(s0, texCoords + vec2( 1.5, -1.5) * kRadius1).rgb;
    bloomSum1 += texture2D(s0, texCoords + vec2(-1.5, -1.5) * kRadius1).rgb;
    bloomSum1 += texture2D(s0, texCoords + vec2( 1.5,  1.5) * kRadius1).rgb;
    bloomSum1 += texture2D(s0, texCoords + vec2(-1.5,  1.5) * kRadius1).rgb;
    bloomSum1 += texture2D(s0, texCoords + vec2( 0.0, -2.5) * kRadius1).rgb;
    bloomSum1 += texture2D(s0, texCoords + vec2( 0.0,  2.5) * kRadius1).rgb;
    bloomSum1 += texture2D(s0, texCoords + vec2(-2.5,  0.0) * kRadius1).rgb;
    bloomSum1 += texture2D(s0, texCoords + vec2( 2.5,  0.0) * kRadius1).rgb;
    bloomSum1 *= 0.005;

    vec3 bloomSum2;
    bloomSum2  = texture2D(s0, texCoords + vec2( 1.5, -1.5) * kRadius2).rgb;
    bloomSum2 += texture2D(s0, texCoords + vec2(-1.5, -1.5) * kRadius2).rgb;
    bloomSum2 += texture2D(s0, texCoords + vec2( 1.5,  1.5) * kRadius2).rgb;
    bloomSum2 += texture2D(s0, texCoords + vec2(-1.5,  1.5) * kRadius2).rgb;
    bloomSum2 += texture2D(s0, texCoords + vec2( 0.0, -2.5) * kRadius2).rgb;
    bloomSum2 += texture2D(s0, texCoords + vec2( 0.0,  2.5) * kRadius2).rgb;
    bloomSum2 += texture2D(s0, texCoords + vec2(-2.5,  0.0) * kRadius2).rgb;
    bloomSum2 += texture2D(s0, texCoords + vec2( 2.5,  0.0) * kRadius2).rgb;
    bloomSum2 *= 0.010;

    vec3 center = texture2D(s0, texCoords).rgb;
    vec3 HDR = (center + (bloomSum2 - bloomSum1)) * kDist;
    vec3 blend = HDR + colorInput;
    vec3 result = pow(abs(blend), kHDRPower) + HDR;

    return clamp(result, vec3(0.0), vec3(1.0));
}

// Makes bright lights bleed their light into their surroundings (relatively high performance cost).
vec3 BloomPass(vec2 texCoords, sampler2D s0, vec3 colorInput)
{
    const float kBloomThreshold = 20.25; // [0.0:50.0] - Threshold for what is a bright light (that causes bloom) and what isn't.
    const float kBloomPower = 1.446;     // [0.0:8.0] - Strength of the bloom.
    const float kBloomWidth = 0.0142;    // [0.0:1.0] - Width of the bloom.
    const float kPixelSize = 1.0;
    const float kMaxDistance = 8.0 * kBloomWidth;
    const float kSamplecount = 25.0;

    vec3 blurColor2 = vec3(0.0);
    vec2 blurTempValue = vec2(texCoords * kPixelSize * kBloomWidth);
    vec2 bloomSample = vec2(2.5, -2.5);
    vec2 bloomSampleValue = bloomSample;

    for (bloomSample.x = 2.5; bloomSample.x > -2.0; bloomSample.x = bloomSample.x - 1.0) // runs 5 times
    {
        bloomSampleValue.x = bloomSample.x * blurTempValue.x;
        vec2 distanceTemp = vec2(bloomSample.x * bloomSample.x * kBloomWidth);

        for (bloomSample.y = -2.5; bloomSample.y < 2.0; bloomSample.y = bloomSample.y + 1.0) // runs 5 (* 5) times
        {
            distanceTemp.y = bloomSample.y * bloomSample.y;
            float curDistance = (distanceTemp.y * kBloomWidth) + distanceTemp.x;

            bloomSampleValue.y = bloomSample.y * blurTempValue.y;
            vec3 blurTemp = texture2D(s0, vec2(texCoords + bloomSampleValue)).rgb;

            blurColor2 += mix(blurTemp, colorInput, sqrt(curDistance / kMaxDistance));
        }
    }

    blurColor2 = (blurColor2 / (kSamplecount - (kBloomPower - kBloomThreshold * 5)));
    float bloomAmount = dot(colorInput, vec3(0.299f, 0.587f, 0.114f));
    vec3 blurColor = blurColor2 * (kBloomPower + 4.0);

    vec3 result = mix(colorInput, blurColor, bloomAmount);
    return clamp(result, vec3(0.0), vec3(1.0));
}

// Scatters the pixels, making the image look fuzzy.
vec3 NoisePass(vec2 texCoords, sampler2D s0, vec3 colorInput)
{
    const float kPixelSize = 100.0;
    const float kNoiseRadius = 100.0; // [0.2:100.0] - Amount of effect you want.

    // Pseudo random number generator:
    float seed = dot(texCoords, vec2(12.9898, 78.233));
    vec2 sineCosine = vec2(sin(seed), cos(seed));
    sineCosine = sineCosine * 43758.5453 + texCoords;
    vec2 noise = fract(sineCosine);

    texCoords = vec2(-kNoiseRadius * kPixelSize) + texCoords;
    vec3 result = texture2D(s0, (2.0 * kNoiseRadius * kPixelSize) * noise + texCoords).rgb;
    result = result + colorInput;

    return clamp(result, vec3(0.0), vec3(1.0));
}
