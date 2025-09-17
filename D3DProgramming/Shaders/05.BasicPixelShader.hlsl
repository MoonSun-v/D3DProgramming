#include "05.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 main(PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;  // 픽셀의 최종 색상 (누적해서 계산)
    
    // 두 개의 광원에 대해 NdotL 조명 계산
    // dot(N, L)  : 법선(N)과 광원 방향(L)의 내적 -> 조명 강도
    // saturate() : 음수값을 0으로 클램핑 (빛이 반대 방향일 경우 음영 처리)
    for (int i = 0; i < 2; i++)
    {
        finalColor += saturate(dot((float3) vLightDir[i], input.Norm) * vLightColor[i]);
    }
    finalColor.a = 1; // 알파값(투명도) = 1 완전 불투명
    return finalColor;
}