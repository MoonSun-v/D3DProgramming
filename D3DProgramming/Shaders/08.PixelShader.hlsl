#include "08.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
    // �ؽ�ó
    float3 texColor = txDiffuse.Sample(samLinear, input.Tex).rgb;
    float3 normalMap = txNormal.Sample(samLinear, input.Tex).rgb;
    float3 specMap = texSpecular.Sample(samLinear, input.Tex).rgb;

    // Normal map�� 0~1 -> -1~1 ��ȯ
    float3 N_tangent = normalize(normalMap * 2.0f - 1.0f);

    // TBN ��� ����
    // float3x3 TBN = float3x3(input.Tangent, input.Bitangent, input.Norm);
    
    // Bitangent ��� (Handedness = 1.0)
    float3 T = normalize(input.Tangent);
    float3 N = normalize(input.Norm);
    float3 B = cross(N, T) * -1.0f; // �޼� ��ǥ��

    // TBN ���
    float3x3 TBN = float3x3(T, B, N);
    
    // Tangent -> World ��ȯ
    float3 N_world = normalize(mul(N_tangent, TBN));
    
    // ���� ��� 
    float3 L = normalize(-vLightDir.xyz);   // Directional Light
    float3 V = normalize(vEyePos.xyz - input.WorldPos);
    float3 H = normalize(L + V);            // Blinn-Phong 
    
    // ���� ��
    float3 ambient = vAmbient.rgb;
    float diff = saturate(dot(N_world, L));
    float3 diffuse = diff * vDiffuse.rgb * vLightColor.rgb;
    
    // Specular ��� 
    float specIntensity = specMap.r; // �Ǵ� (specMap.r + specMap.g + specMap.b)/3.0f
    float spec = pow(saturate(dot(N_world, H)), fShininess) * specIntensity;
    float3 specular = spec * vSpecular.rgb * vLightColor.rgb;

    // ���� ����
    float3 finalColor = (ambient + diffuse + specular) * texColor;

    return float4(finalColor, 1.0f);
}
