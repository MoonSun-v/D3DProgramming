#include "08.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // ���� ��ȯ��(Local �� World �� View �� Projection)
    float4 worldPos = mul(input.Pos, World);
    output.Pos = mul(worldPos, View);
    output.Pos = mul(output.Pos, Projection);

    output.WorldPos = worldPos.xyz;

    // ���� ���� ����
    float3x3 world3x3 = (float3x3) World;
    output.Norm = normalize(mul(input.Norm, world3x3));

    // ���� ���� Tangent & Bitangent
    output.Tangent = normalize(mul(input.Tangent, world3x3));
    // output.Bitangent = cross(output.Norm, output.Tangent); // ������ ��ǥ��

    // �ؽ�ó ����
    output.Tex = input.Tex;
    
    return output;
}