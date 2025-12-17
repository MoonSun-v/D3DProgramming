#include "16.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
VSOut main(uint id : SV_VertexID)
{
    VSOut o;

    // Fullscreen triangle
    float2 pos[3] =
    {
        float2(-1, -1),
        float2(-1, 3),
        float2(3, -1)
    };

    float2 uv[3] =
    {
        float2(0, 1),
        float2(0, -1),
        float2(1, 1)
    };

    o.Pos = float4(pos[id], 0, 1);
    o.UV = uv[id];

    return o;
}