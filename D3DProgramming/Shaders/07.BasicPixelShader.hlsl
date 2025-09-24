#include "07.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

// DotProduct�� ������ �����Ƿ� saturate �� max ����ؾ� �� 
float4 main(PS_INPUT input) : SV_Target
{
    // �ؽ�ó ����
    float3 texColor = txDiffuse.Sample(samLinear, input.Tex).rgb;

    // N : Normal vector    �ȼ��� ��� ����
    // L : Light vector     �ȼ� ��ġ���� ����Ʈ ��ġ������ ���� (����Ʈ ����Ʈ) 
    // V : View vector      �ȼ� ��ġ���� ī�޶�(eye) ��ġ������ ����
    float3 N = normalize(input.Norm);
    float3 L = normalize(vLightDir.xyz - input.WorldPos);
    float3 V = normalize(vEyePos.xyz - input.WorldPos);

    
    // [ Blinn-Phong ] 
    // R (�ݻ纤��) ��� H (Half-vector) ���
    float3 H = normalize(L + V); // H = L�� V�� ���� ����ȭ�� ���� -> ����Ʈ ����� �� ������ �߰� ����

    
    // Ambient �⺻ �ֺ���
    // Diffuse Lambertian ��
    // Specular Blinn-Phong ��, dot(N,H)�� shininess�� ����
    float3 ambient = vAmbient.rgb;
    
    float diff = saturate(dot(N, L));
    float3 diffuse = diff * vDiffuse.rgb * vLightColor.rgb;
    
    float spec = pow(saturate(dot(N, H)), fShininess);
    float3 specular = spec * vSpecular.rgb * vLightColor.rgb;

    // ���� ����
    float3 finalColor = (ambient + diffuse + specular) * texColor;

    return float4(finalColor, 1.0f);
}
