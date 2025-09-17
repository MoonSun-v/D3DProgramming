#include "05.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 main(PS_INPUT input) : SV_Target
{
    float4 finalColor = 0;  // �ȼ��� ���� ���� (�����ؼ� ���)
    
    // �� ���� ������ ���� NdotL ���� ���
    // dot(N, L)  : ����(N)�� ���� ����(L)�� ���� -> ���� ����
    // saturate() : �������� 0���� Ŭ���� (���� �ݴ� ������ ��� ���� ó��)
    for (int i = 0; i < 2; i++)
    {
        finalColor += saturate(dot((float3) vLightDir[i], input.Norm) * vLightColor[i]);
    }
    finalColor.a = 1; // ���İ�(����) = 1 ���� ������
    return finalColor;
}