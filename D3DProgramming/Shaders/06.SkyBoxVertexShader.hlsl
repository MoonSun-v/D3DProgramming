#include "06.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT_SKY main(VS_INPUT_SKY input)
{
    PS_INPUT_SKY output = (PS_INPUT_SKY) 0;
    
    // ī�޶� ��ġ ����
    matrix viewNoTrans = View;
    viewNoTrans._41 = 0;
    viewNoTrans._42 = 0;
    viewNoTrans._43 = 0;

    // clip space ��ǥ
    output.Pos = mul(input.Pos, viewNoTrans);
    output.Pos = mul(output.Pos, Projection);

    // ���� ���� ���� (����ȭ)
    output.WorldDir = normalize(mul(input.Pos.xyz, (float3x3) World));
    
    
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