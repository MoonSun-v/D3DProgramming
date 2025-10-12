#include "08.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output = (PS_INPUT) 0;

    // 월드 변환　(Local → World → View → Projection)
    float4 worldPos = mul(input.Pos, World);
    output.Pos = mul(worldPos, View);
    output.Pos = mul(output.Pos, Projection);

    output.WorldPos = worldPos.xyz;

    // 월드 공간 법선
    float3x3 world3x3 = (float3x3) World;
    output.Norm = normalize(mul(input.Norm, world3x3));

    // 월드 공간 Tangent & Bitangent
    output.Tangent = normalize(mul(input.Tangent, world3x3));
    // output.Bitangent = cross(output.Norm, output.Tangent); // 오른손 좌표계

    // 텍스처 전달
    output.Tex = input.Tex;
    
    return output;
}