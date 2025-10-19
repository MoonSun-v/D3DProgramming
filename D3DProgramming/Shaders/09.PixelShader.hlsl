#include "09.Shared.hlsli"

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
    // [1] 텍스처 샘플링
    float3 texColor = UseDiffuse ? txDiffuse.Sample(samLinear, input.Tex).rgb : vDiffuse.rgb;
    float3 normalMap = UseNormal ? txNormal.Sample(samLinear, input.Tex).rgb : EncodeNormal(input.Norm);
    float3 specMap = UseSpecular ? txSpecular.Sample(samLinear, input.Tex).rgb : float3(1, 1, 1);
    float3 emissive = UseEmissive ? txEmissive.Sample(samLinear, input.Tex).rgb : float3(0, 0, 0);
    float opacity = UseOpacity ? txOpacity.Sample(samLinear, input.Tex).r : 1.0f;
    
    // [2] Normal 계산
    float3 N_tangent = normalize(DecodeNormal(normalMap));
    float3 T = normalize(input.Tangent);
    float3 B = normalize(input.Binormal);
    float3 N = normalize(input.Norm);
    float3x3 TBN = float3x3(T, B, N);
    float3 N_world = normalize(mul(N_tangent, TBN)); // Tangent Space -> World Space 변환
    
    // [3] Blinn-Phong 조명 계산 
    float3 L = normalize(-vLightDir.xyz); // Directional Light
    float3 V = normalize(vEyePos.xyz - input.WorldPos);
    float3 H = normalize(L + V); // Blinn-Phong 

    // Ambient
    float3 ambient = vAmbient.rgb;

    // Diffuse
    float diff = saturate(dot(N_world, L));
    float3 diffuse = diff * vLightColor.rgb * vDiffuse.rgb;

    // Specular
    float specIntensity = specMap.r;
    float spec = pow(saturate(dot(N_world, H)), fShininess) * specIntensity;
    float3 specular = spec * vLightColor.rgb * vSpecular.rgb;
    
    // [4] Emissive + 합산
    float3 finalColor = (ambient + diffuse + specular) * texColor + emissive;

    return float4(finalColor, opacity);
}


