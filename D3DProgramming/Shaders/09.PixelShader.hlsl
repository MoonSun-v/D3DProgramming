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
    // 1. �ؽ�ó ���ø� : C++���� ������ �⺻ �ؽ��� ������ ���� 
    float4 texDiffColor = txDiffuse.Sample(samLinear, input.Tex);
    float4 texNormColor = txNormal.Sample(samLinear, input.Tex);
    float4 texSpecColor = txSpecular.Sample(samLinear, input.Tex);
    float4 texEmissive = txEmissive.Sample(samLinear, input.Tex);
    float4 texOpacColor = txOpacity.Sample(samLinear, input.Tex);

    
    // 2. Opacity Clipping (���� �׽�Ʈ)
    // ���� ���� 0.5���� ������ �ȼ��� �׸��� �ʰ� ����
    clip(texOpacColor.a - 0.5f); 

    
    // 3. Normal ��� (TBN �������� ���� ��������)
    float3 norm = normalize(input.Norm);
    float3 tan = normalize(input.Tangent);
    float3 binorm = normalize(input.Binormal);
    float3x3 tbnMatrix = float3x3(tan, binorm, norm);
    
    // ��� �ʿ��� ���� ������ ���� ��� ���ͷ� ���ڵ�
    float3 texNorm = DecodeNormal(texNormColor.rgb);
    float3 worldNorm = normalize(mul(texNorm, tbnMatrix));

    
    // 4. Ambient ( diffuse ���� * ��Ƽ���� ambient �� * ����Ʈ ���� )
    float4 ambient = texDiffColor * vAmbient * vLightColor;

    
    // 5. Diffuse (Lambert)
    float NdotL = max(dot(worldNorm, -vLightDir.xyz), 0.0f);
    float4 diffuse = texDiffColor * vLightColor * NdotL;

    
    // 6. Specular (Blinn-Phong)
    float3 viewDir = normalize(vEyePos.xyz - input.WorldPos);
    float3 halfVector = normalize(-vLightDir.xyz + viewDir);
    float specularFactor = max(dot(worldNorm, halfVector), 0.0f);
    float4 specular = texSpecColor * vSpecular * vLightColor * pow(specularFactor, fShininess);

    
    // 7. Emissive : ��ü �߱�!
    float4 emissive = texEmissive;

    
    // 8. ���� ���� �ջ�
    float4 finalColor = saturate(diffuse + ambient + specular + emissive);
    
     // Alpha ä���� opacity �ؽ�ó ���
    finalColor.a = texOpacColor.a;

    return finalColor;
}





