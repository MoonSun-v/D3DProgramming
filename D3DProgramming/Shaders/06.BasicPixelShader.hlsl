#include "06.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
     // �ؽ�ó ���ø�
    float4 texColor = txDiffuse.Sample(samLinear, input.Tex);

    // ���� ��� (Diffuse)
    float3 L = normalize(vLightDir.xyz); // ����Ʈ ���� ����ȭ
    float NdotL = saturate(dot(input.Norm, L)); // ������ ����Ʈ ���� ����
    float3 finalColor = texColor.rgb * vLightColor.rgb * NdotL; // ��ǻ�� ����
    
    return float4(finalColor, 1.0f);
}
