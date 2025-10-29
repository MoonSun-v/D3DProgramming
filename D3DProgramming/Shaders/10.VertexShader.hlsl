#include "10.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;
    
    float4 posBone = input.Pos;
    
    // Rigid: 월드 행렬만 적용
    //if (gIsRigid == 1)
    //{
    //    posBone = mul(posBone, gModelMatricies[0]);
    //    // posBone = mul(gModelMatricies[0], input.Pos );
    //}
    
    // Model -> World 
    float4 worldPos = mul(posBone, gWorld); 
    // float4 worldPos = mul(input.Pos, gWorld);
    
    // World -> View -> Projection
    output.Pos = mul(worldPos, gView);
    output.Pos = mul(output.Pos, gProjection);

    output.WorldPos = worldPos.xyz;

    // 월드 공간 변환용 3x3 행렬
    float3x3 world3x3 = (float3x3) gWorld;

    // 월드 법선, Tangent, Binormal 변환
    output.Norm = normalize(mul(input.Norm, world3x3));
    output.Tangent = normalize(mul(input.Tangent, world3x3));
    output.Binormal = normalize(mul(input.Binormal, world3x3));

    // 텍스처 좌표 전달
    output.Tex = input.Tex;
    
    return output;
}