#include "10.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
 
    
    // CPU���� �̹� SubMesh �������� RefBone ���� ��ȯ�� ����Ǿ� �����Ƿ�
    // ������ �� ��� ���� �ʿ� ����..! 
    float4 posBone = input.Pos;

    // Model -> World
    float4 worldPos = mul(posBone, gWorld);
    
    // World -> View -> Projection
    output.Pos = mul(worldPos, gView);
    output.Pos = mul(output.Pos, gProjection);

    output.WorldPos = worldPos.xyz;

    // ���� ���� ��ȯ�� 3x3 ���
    float3x3 world3x3 = (float3x3) gWorld;

    // ���� ����, Tangent, Binormal ��ȯ
    output.Norm = normalize(mul(input.Norm, world3x3));
    output.Tangent = normalize(mul(input.Tangent, world3x3));
    output.Binormal = normalize(mul(input.Binormal, world3x3));

    // �ؽ�ó ��ǥ ����
    output.Tex = input.Tex;
    
    return output;
}