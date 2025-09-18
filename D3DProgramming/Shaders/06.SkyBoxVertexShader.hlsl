#include "06.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT_SKY main(VS_INPUT_SKY input)
{
    PS_INPUT_SKY output = (PS_INPUT_SKY) 0;
    
    // 카메라 위치 제거
    matrix viewNoTrans = View;
    viewNoTrans._41 = 0;
    viewNoTrans._42 = 0;
    viewNoTrans._43 = 0;

    // clip space 좌표
    output.Pos = mul(input.Pos, viewNoTrans);
    output.Pos = mul(output.Pos, Projection);

    // 월드 방향 벡터 (정규화)
    output.WorldDir = normalize(mul(input.Pos.xyz, (float3x3) World));
    
    
    // [ 비균등 스케일 문제 ] 주의 !!! 
    // 
    // World 행렬에 스케일/이동이 있으면 단위 벡터가 깨질 수 있음
    // output.Norm = mul(float4(input.Norm, 1), World).xyz;    // 이렇게 하면 안됨 
    // 
    // 정석의 방법 : 역전치 행렬(Transpose(Inverse(World)))을 쓰는 것
    // 여기에서는  : World 행렬의 3x3 회전/스케일 성분만 적용 후 normalize
    //              이동 성분 제외 + 스케일 왜곡 보정
    //
    // World Matrix에서 이동성분을 제외하고 적용하며,  scale 있을수 있으므로 normalize 사용한다.
    // output.Norm = normalize(mul(input.Norm, (float3x3) World));
   
    // input.Norm           : Local(모델) 공간의 법선
    // (float3x3)World      : World 행렬의    상위3 x3만    추출(회전 + 스케일)
    
    return output;
}