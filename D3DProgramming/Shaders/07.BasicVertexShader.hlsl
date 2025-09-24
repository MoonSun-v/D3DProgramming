#include "07.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // ���� ��ȯ��(Local �� World �� View �� Projection)
    output.Pos = mul(input.Pos, World);
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);

    // ���� ����������յ� ������ ������ ����
    output.Norm = normalize(mul(input.Norm, (float3x3) World));

    // �ؽ�ó ����
    output.Tex = input.Tex;
    
    
    // [ ��յ� ������ ���� ] ���� !!! 
    // 
    // World ��Ŀ� ������/�̵��� ������ ���� ���Ͱ� ���� �� ����
    // output.Norm = mul(float4(input.Norm, 1), World).xyz;    // �̷��� �ϸ� �ȵ� 
    // 
    // ������ ��� : ����ġ ���(Transpose(Inverse(World)))�� ���� ��
    // ���⿡����  : World ����� 3x3 ȸ��/������ ���и� ���� �� normalize
    //              �̵� ���� ���� + ������ �ְ� ����
    //
    // World Matrix���� �̵������� �����ϰ� �����ϸ�,  scale ������ �����Ƿ� normalize ����Ѵ�.
    // output.Norm = normalize(mul(input.Norm, (float3x3) World));
   
    // input.Norm           : Local(��) ������ ����
    // (float3x3)World      : World �����    ����3 x3��    ����(ȸ�� + ������)
    
    return output;
}