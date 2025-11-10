#include "11.ToonShared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 OutlinePS(VS_OUTLINE_OUTPUT input) : SV_Target
{
    return input.Color;
}
