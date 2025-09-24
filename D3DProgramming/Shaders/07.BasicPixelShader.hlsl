#include "07.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

// DotProduct는 음수가 나오므로 saturate 나 max 사용해야 함 
float4 main(PS_INPUT input) : SV_Target
{
    // 텍스처 색상
    float3 texColor = txDiffuse.Sample(samLinear, input.Tex).rgb;

    // N : Normal vector    픽셀의 노멀 벡터
    // L : Light vector     픽셀 위치에서 라이트 위치까지의 벡터 (포인트 라이트) 
    // V : View vector      픽셀 위치에서 카메라(eye) 위치까지의 벡터
    float3 N = normalize(input.Norm);
    float3 L = normalize(vLightDir.xyz - input.WorldPos);
    float3 V = normalize(vEyePos.xyz - input.WorldPos);

    
    // [ Blinn-Phong ] 
    // R (반사벡터) 대신 H (Half-vector) 사용
    float3 H = normalize(L + V); // H = L과 V의 합을 정규화한 벡터 -> 라이트 방향과 뷰 방향의 중간 방향

    
    // Ambient 기본 주변광
    // Diffuse Lambertian 모델
    // Specular Blinn-Phong 모델, dot(N,H)를 shininess로 제곱
    float3 ambient = vAmbient.rgb;
    
    float diff = saturate(dot(N, L));
    float3 diffuse = diff * vDiffuse.rgb * vLightColor.rgb;
    
    float spec = pow(saturate(dot(N, H)), fShininess);
    float3 specular = spec * vSpecular.rgb * vLightColor.rgb;

    // 최종 색상
    float3 finalColor = (ambient + diffuse + specular) * texColor;

    return float4(finalColor, 1.0f);
}
