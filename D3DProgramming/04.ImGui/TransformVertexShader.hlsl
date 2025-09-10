// [ 버택스 셰이더 ]
#include <shared.fxh>

// 회전, 스케일은 행렬 곱으로 표현 가능하지만, 이동은 단순한 덧셈이므로 행렬 곱으로 표현 X
// -> 동차 좌표 활용하면 이동도 행렬 곱으로 표현할 수 있음 

// 동차 좌표 : (x, y, z)에 w=1 을 붙여서 4차원 벡터로 만든 것


PS_INPUT main(float4 Pos : POSITION, float4 Color : COLOR)
{
    PS_INPUT output = (PS_INPUT) 0;
    output.Pos = mul(Pos, World);               // Vector4(WorldPos) = vector4(ModelPos) * Matrix4x4(WorldTransform)  
    output.Pos = mul(output.Pos, View);
    output.Pos = mul(output.Pos, Projection);
    output.Color = Color;
    return output;
}