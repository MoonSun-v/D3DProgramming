#include "17.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Shadow Pixel Shader
//--------------------------------------------------------------------------------------

float ShadowPS(VS_SHADOW_OUT input) : SV_Depth
{
    // 알파 텍스처 샘플링
    float alpha = txOpacity.Sample(samLinear, input.Tex).a;
    
    clip(alpha - 0.5f);
    
    return input.Pos.z; 
}