#include "07.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

// DotProduct는 음수가 나오므로 saturate 나 max 사용해야 함 
float4 main(PS_INPUT input) : SV_Target
{
    // 텍스처 색상
    float3 texColor = txDiffuse.Sample(samLinear, input.Tex).rgb;

    // Normal
    float3 N = normalize(input.Norm);

    // Light vector (포인트라이트)
    float3 L = normalize(vLightDir.xyz - input.WorldPos);

    // View vector
    float3 V = normalize(vEyePos.xyz - input.WorldPos);

    // Half vector (Blinn-Phong)
    float3 H = normalize(L + V);

    // Ambient
    float3 ambient = vAmbient.rgb;

    // Diffuse
    float diff = saturate(dot(N, L));
    float3 diffuse = diff * vDiffuse.rgb * vLightColor.rgb;

    // Specular
    float spec = pow(saturate(dot(N, H)), fShininess);
    float3 specular = spec * vSpecular.rgb * vLightColor.rgb;

    // 최종 색상
    float3 finalColor = (ambient + diffuse + specular) * texColor;

    
    return float4(finalColor, 1.0f);
}
