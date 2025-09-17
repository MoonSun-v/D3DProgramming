#include "06.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
     // 텍스처 샘플링
    float4 texColor = txDiffuse.Sample(samLinear, input.Tex);

    // 조명 계산 (Diffuse)
    float3 L = normalize(vLightDir.xyz); // 라이트 방향 정규화
    float NdotL = saturate(dot(input.Norm, L)); // 법선과 라이트 방향 내적
    float3 finalColor = texColor.rgb * vLightColor.rgb * NdotL; // 디퓨즈 조명
    
    return float4(finalColor, 1.0f);
}
