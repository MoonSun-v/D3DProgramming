#include "09.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // ���� ��ȯ��(Local -> World -> View -> Projection)
    float4 worldPos = mul(input.Pos, World);
    output.Pos = mul(worldPos, View);
    output.Pos = mul(output.Pos, Projection);

    output.WorldPos = worldPos.xyz;

    // ���� ���� ��ȯ�� 3x3 ���
    float3x3 world3x3 = (float3x3) World;

    // ���� ����, Tangent, Binormal ��ȯ
    output.Norm = normalize(mul(input.Norm, world3x3));
    output.Tangent = normalize(mul(input.Tangent, world3x3));
    output.Binormal = normalize(mul(input.Binormal, world3x3));

    // �ؽ�ó ��ǥ ����
    output.Tex = input.Tex;
    
    return output;
}