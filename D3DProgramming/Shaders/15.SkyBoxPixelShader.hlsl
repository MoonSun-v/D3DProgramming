#include "15.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT_SKY input) : SV_Target
{
    return txSky.Sample(samLinear, input.WorldDir);
}
