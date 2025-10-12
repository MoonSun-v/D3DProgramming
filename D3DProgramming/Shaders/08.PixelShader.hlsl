#include "08.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
    // 텍스처
    float3 texColor = txDiffuse.Sample(samLinear, input.Tex).rgb;
    float3 normalMap = txNormal.Sample(samLinear, input.Tex).rgb;
    float3 specMap = texSpecular.Sample(samLinear, input.Tex).rgb;

    // Normal map은 0~1 -> -1~1 변환
    float3 N_tangent = normalize(normalMap * 2.0f - 1.0f);

    // TBN 행렬 구성
    // float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Norm);
    
    // Bitangent 계산 (Handedness = 1.0)
    float3 T = normalize(input.Tangent);
    float3 N = normalize(input.Norm);
    float3 B = cross(N, T) * -1.0f; // 왼손 좌표계

    // TBN 행렬
    float3x3 TBN = float3x3(T, B, N);
    
    // Tangent -> World 변환
    float3 N_world = normalize(mul(N_tangent, TBN));
    
    // 광원 계산 
    float3 L = normalize(-vLightDir.xyz);   // Directional Light
    float3 V = normalize(vEyePos.xyz - input.WorldPos);
    float3 H = normalize(L + V);            // Blinn-Phong 
    
    // 조명 모델
    float3 ambient = vAmbient.rgb;
    float diff = saturate(dot(N_world, L));
    float3 diffuse = diff * vDiffuse.rgb * vLightColor.rgb;
    
    // Specular 계산 
    float specIntensity = specMap.r; // 또는 (specMap.r + specMap.g + specMap.b)/3.0f
    float spec = pow(saturate(dot(N_world, H)), fShininess) * specIntensity;
    float3 specular = spec * vSpecular.rgb * vLightColor.rgb;

    // 최종 색상
    float3 finalColor = (ambient + diffuse + specular) * texColor;

    return float4(finalColor, 1.0f);
}
