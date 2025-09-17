#include "05.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);

    // ����Ʈ ���� ����ȭ
    float3 L = normalize(vLightDir.xyz);

    // ������ ����Ʈ ���� ����
    float NdotL = saturate(dot(input.Norm, L));

    // ��ǻ�� ����
    float3 finalColor = baseColor * vLightColor.rgb * NdotL;

    return float4(finalColor, 1.0f);
}

//float4 main(PS_INPUT input) : SV_Target
//{
//    float4 finalColor = 0;  // �ȼ��� ���� ���� (�����ؼ� ���)
    
//    // �� ���� ������ ���� NdotL ���� ���
//    // dot(N, L)  : ����(N)�� ���� ����(L)�� ���� -> ���� ����
//    // saturate() : �������� 0���� Ŭ���� (���� �ݴ� ������ ��� ���� ó��)
//    for (int i = 0; i < 2; i++)
//    {
//        finalColor += saturate(dot((float3) vLightDir[i], input.Norm) * vLightColor[i]);
//    }
//    finalColor.a = 1; // ���İ�(����) = 1 ���� ������
//    return finalColor;
//}