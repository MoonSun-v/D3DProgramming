#include "17.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Shadow Pixel Shader
//--------------------------------------------------------------------------------------

float ShadowPS(PS_INPUT input) : SV_Depth
{
    // 알파 텍스처 샘플링
    float alpha = txOpacity.Sample(samLinear, input.Tex).a;
    
    clip(alpha - 0.5f);

    // Depth는 자동으로 SV_Depth에 기록됨
    return input.Pos.z;
}