#include "06.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------

float4 main(PS_INPUT input) : SV_Target
{
    float3 baseColor = float3(1.0f, 1.0f, 1.0f);

    float3 L = normalize(vLightDir.xyz); // ����Ʈ ���� ����ȭ
    
    float NdotL = saturate(dot(input.Norm, L)); // ������ ����Ʈ ���� ����
    
    float3 finalColor = baseColor * vLightColor.rgb * NdotL; // ��ǻ�� ����

    return float4(finalColor, 1.0f);
}
