// [ ���ý� ���̴� ]
#include <shared.fxh>

// ȸ��, �������� ��� ������ ǥ�� ����������, �̵��� �ܼ��� �����̹Ƿ� ��� ������ ǥ�� X
// -> ���� ��ǥ Ȱ���ϸ� �̵��� ��� ������ ǥ���� �� ���� 

// ���� ��ǥ : (x, y, z)�� w=1 �� �ٿ��� 4���� ���ͷ� ���� ��


PS_INPUT main(float4 Pos : POSITION, float4 Color : COLOR)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = mul(Pos, World);               // Vector4(WorldPos) = vector4(ModelPos) * Matrix4x4(WorldTransform)  
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Color = Color;
    return output;
}