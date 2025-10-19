#include "09.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // 월드 변환　(Local -> World -> View -> Projection)
    float4 worldPos = mul(input.Pos, World);
    output.Pos = mul(worldPos, View);
    output.Pos = mul(output.Pos, Projection);

    output.WorldPos = worldPos.xyz;

    // 월드 공간 변환용 3x3 행렬
    float3x3 world3x3 = (float3x3) World;

    // 월드 법선, Tangent, Binormal 변환
    output.Norm = normalize(mul(input.Norm, world3x3));
    output.Tangent = normalize(mul(input.Tangent, world3x3));
    output.Binormal = normalize(mul(input.Binormal, world3x3));

    // 텍스처 좌표 전달
    output.Tex = input.Tex;
    
    return output;
}