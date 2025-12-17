#include "16.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float3 ACESFilm(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_Target
{
    float3 hdr = txSceneHDR.Sample(samLinear, uv).rgb;

    hdr *= Exposure; // Exposure
    float3 ldr = ACESFilm(hdr); // Tone Mapping
    ldr = pow(ldr, 1.0 / 2.2); // Gamma

    return float4(ldr, 1.0);
}