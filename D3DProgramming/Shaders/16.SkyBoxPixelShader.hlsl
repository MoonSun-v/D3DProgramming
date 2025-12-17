#include "16.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT_SKY input) : SV_Target
{
    float3 sky = txSky.Sample(samLinear, input.WorldDir).rgb;

    // Skybox 전용 노출 (보통 -2 ~ -4 EV)
    sky *= pow(2.0f, -2.0f/*gSkyExposureEV*/);

    return float4(sky, 1.0);
}
