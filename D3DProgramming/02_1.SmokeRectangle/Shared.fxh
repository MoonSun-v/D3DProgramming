// Shared.fxh

cbuffer TimeBuffer : register(b0)
{
    float time;
    float3 padding; 
}

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 uv : TEXCOORD0; // Vertex Shader에서 UV 전달
};
