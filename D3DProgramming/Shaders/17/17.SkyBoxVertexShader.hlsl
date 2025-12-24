#include "17.Shared.hlsli"

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

    // Skybox는 월드 = 카메라 위치
    float4 worldPos = float4(input.Pos.xyz + vEyePos.xyz, 1.0f);

    // View / Projection
    output.Pos = mul(worldPos, viewNoTrans);
    output.Pos = mul(output.Pos, gProjection);

    // Cubemap 방향
    output.WorldDir = input.Pos.xyz;

    return output;
}