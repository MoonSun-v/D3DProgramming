#include "08.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float3 EncodeNormal(float3 N)
{
    return N * 0.5 + 0.5;
}
float3 DecodeNormal(float3 N)
{
    return N * 2 - 1;
}

float4 main(PS_INPUT input) : SV_Target
{
    // 텍스처 샘플링 : input.Tex : 각 픽셀의 UV좌표 (0~1)
    float3 texColor = txDiffuse.Sample(samLinear, input.Tex).rgb;
    float3 normalMap = txNormal.Sample(samLinear, input.Tex).rgb;
    float3 specMap = texSpecular.Sample(samLinear, input.Tex).rgb;
    
    // Normal map 디코딩 (0~1 -> -1~1)
    float3 N_tangent = normalize(DecodeNormal(normalMap));
    
    // Binormal은 CPU에서 이미 지정되어 있음 (따로 계산X)
    float3 T = normalize(input.Tangent);
    float3 B = normalize(input.Binormal);
    float3 N = normalize(input.Norm);

    // TBN 3x3 행렬 생성
    float3x3 TBN = float3x3(T, B, N);
    
    // Tangent Space -> World Space 변환
    float3 N_world = normalize(mul(N_tangent, TBN));
    
    // 광원 계산 
    float3 L = normalize(-vLightDir.xyz);   // Directional Light
    float3 V = normalize(vEyePos.xyz - input.WorldPos);
    float3 H = normalize(L + V);            // Blinn-Phong 
    
    // Ambient + Diffuse 계산
    float3 ambient = vAmbient.rgb;
    float diff = saturate(dot(N_world, L));
    float3 diffuse = diff * vDiffuse.rgb * vLightColor.rgb;
    
    // Specular 계산 
    float specIntensity = specMap.r; 
    float spec = pow(saturate(dot(N_world, H)), fShininess) * specIntensity;
    float3 specular = spec * vSpecular.rgb * vLightColor.rgb;

    // 최종 색상
    float3 finalColor = (ambient + diffuse + specular) * texColor;

    return float4(finalColor, 1.0f);
}


