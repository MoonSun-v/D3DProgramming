#include "06.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT_SKY input) : SV_Target
{
    return g_SkyTexture.Sample(samLinear, input.WorldDir);
}
