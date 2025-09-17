#include "05.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);

    // 라이트 방향 정규화
    float3 L = normalize(vLightDir.xyz);

    // 법선과 라이트 방향 내적
    float NdotL = saturate(dot(input.Norm, L));

    // 디퓨즈 조명
    float3 finalColor = baseColor * vLightColor.rgb * NdotL;

    return float4(finalColor, 1.0f);
}
