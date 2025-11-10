#include "11.ToonShared.hlsli"

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
// 1. 텍스처 샘플링
    float4 texDiffColor = txDiffuse.Sample(samLinear, input.Tex);
    float4 texNormColor = txNormal.Sample(samLinear, input.Tex);
    float4 texSpecColor = txSpecular.Sample(samLinear, input.Tex);
    float4 texEmissive = txEmissive.Sample(samLinear, input.Tex);
    float4 texOpacColor = txOpacity.Sample(samLinear, input.Tex);
    clip(texOpacColor.a - 0.5f);

    // 2. 노멀 계산
    float3 norm = normalize(input.Norm);
    float3 tan = normalize(input.Tangent);
    float3 binorm = normalize(input.Binormal);
    float3x3 tbn = float3x3(tan, binorm, norm);
    float3 texNormal = DecodeNormal(texNormColor.rgb);
    float3 worldNorm = normalize(mul(texNormal, tbn));

    // 3. 기본 벡터
    float3 L = normalize(-vLightDir.xyz);
    float3 V = normalize(vEyePos.xyz - input.WorldPos);
    float3 R = normalize(2.0 * dot(worldNorm, L) * worldNorm - L);

    // 4. Toon Diffuse
    float NdotL = saturate(dot(worldNorm, L));
    float2 rampUV = float2(NdotL, 0.5);
    float4 rampColor = txRamp.Sample(samLinear, rampUV);
    float4 toonDiffuse = rampColor * texDiffColor * vLightColor;

    // 5. Toon Specular
    float specIntensity = pow(saturate(dot(R, V)), fShininess);
    float2 specUV = float2(specIntensity, 0.5);
    float4 specRampColor = txSpecularRamp.Sample(samLinear, specUV);
    float4 toonSpecular = specRampColor * texSpecColor * vSpecular * vLightColor;

    // 6. Ambient + Emissive
    float4 ambient = texDiffColor * vAmbient * vLightColor;
    float4 emissive = texEmissive;

    // 7. 합성
    float4 finalColor = saturate(toonDiffuse + ambient + toonSpecular + emissive);
    finalColor.a = texOpacColor.a;

    return finalColor;
}






