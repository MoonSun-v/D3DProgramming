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
    // �ؽ�ó ���ø� : input.Tex : �� �ȼ��� UV��ǥ (0~1)
    float3 texColor = txDiffuse.Sample(samLinear, input.Tex).rgb;
    float3 normalMap = txNormal.Sample(samLinear, input.Tex).rgb;
    float3 specMap = texSpecular.Sample(samLinear, input.Tex).rgb;
    
    // Normal map ���ڵ� (0~1 -> -1~1)
    float3 N_tangent = normalize(DecodeNormal(normalMap));
    
    // Binormal�� CPU���� �̹� �����Ǿ� ���� (���� ���X)
    float3 T = normalize(input.Tangent);
    float3 B = normalize(input.Binormal);
    float3 N = normalize(input.Norm);

    // TBN 3x3 ��� ����
    float3x3 TBN = float3x3(T, B, N);
    
    // Tangent Space -> World Space ��ȯ
    float3 N_world = normalize(mul(N_tangent, TBN));
    
    // ���� ��� 
    float3 L = normalize(-vLightDir.xyz);   // Directional Light
    float3 V = normalize(vEyePos.xyz - input.WorldPos);
    float3 H = normalize(L + V);            // Blinn-Phong 
    
    // Ambient + Diffuse ���
    float3 ambient = vAmbient.rgb;
    float diff = saturate(dot(N_world, L));
    float3 diffuse = diff * vDiffuse.rgb * vLightColor.rgb;
    
    // Specular ��� 
    float specIntensity = specMap.r; 
    float spec = pow(saturate(dot(N_world, H)), fShininess) * specIntensity;
    float3 specular = spec * vSpecular.rgb * vLightColor.rgb;

    // ���� ����
    float3 finalColor = (ambient + diffuse + specular) * texColor;

    return float4(finalColor, 1.0f);
}


