#include "16.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT_SKY main(VS_INPUT_SKY input)
{
    PS_INPUT_SKY output = (PS_INPUT_SKY) 0;
    
    // 카메라 위치 제거
    matrix viewNoTrans = gView;
    viewNoTrans._41 = 0;
    viewNoTrans._42 = 0;
    viewNoTrans._43 = 0;

    // clip space 좌표
    output.Pos = mul(input.Pos, viewNoTrans);
    output.Pos = mul(output.Pos, gProjection);

    // 월드 방향 벡터 (정규화)
    output.WorldDir = normalize(input.Pos.xyz);
    
    return output;
}