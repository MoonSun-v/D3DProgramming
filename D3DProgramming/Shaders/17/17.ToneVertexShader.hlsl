#include "17.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Tone Vertex Shader
//--------------------------------------------------------------------------------------
VS_OUTPUT_FULL main(uint id : SV_VertexID)
{
    VS_OUTPUT_FULL o;

    float2 pos[3] =
    {
        float2(-1.0, -1.0),
        float2(-1.0, 3.0),
        float2(3.0, -1.0)
    };

    o.Pos = float4(pos[id], 0.0, 1.0);

    // NDC ¡æ UV º¯È¯
    o.UV = o.Pos.xy * float2(0.5, -0.5) + 0.5;

    return o;
}