#include "16.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader (LDR 출력용)
//--------------------------------------------------------------------------------------

float3 LinearToSRGB(float3 linearColor)
{
    return pow(linearColor, 1.0f / 2.2f);
}

float4 main(float4 pos : SV_POSITION, float2 uv : TEXCOORD0) : SV_Target
{
    // 1. Scene HDR : 선형 HDR 값 로드 (Nits 값으로 간주)
    float3 hdr709 = txSceneHDR.Sample(samLinear, uv).rgb;

    // 2. Exposure (EV)
    float exposureFactor = pow(2.0f, gExposure);
    hdr709 *= exposureFactor;

    // 3. Tone Mapping (ACES)
    float3 tonemapped = ACESFilm(hdr709);

    // 4. PQ로 변환 (HDR10 출력용)
    float3 pq = LinearToSRGB(tonemapped);

    // 5. 최종 PQ 인코딩된 값 [0.0, 1.0]을 R10G10B10A2_UNORM 백버퍼에 출력
    return float4(pq, 1.0);
}