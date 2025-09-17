#include "06.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);

    float3 L = normalize(vLightDir.xyz); // 라이트 방향 정규화
    
    float NdotL = saturate(dot(input.Norm, L)); // 법선과 라이트 방향 내적
    
    float3 finalColor = baseColor * vLightColor.rgb * NdotL; // 디퓨즈 조명

    return float4(finalColor, 1.0f);
}
