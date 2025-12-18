#include <Shared.fxh>

//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------

PS_INPUT main(float4 pos : POSITION, float4 color : COLOR)
{
    PS_INPUT output;
    output.pos = pos;
    output.color = color;
    
    // NDC(-1~1) -> 0~1 UV °è»ê
    output.uv = pos.xy * 0.5 + 0.5;
    
    return output;
}