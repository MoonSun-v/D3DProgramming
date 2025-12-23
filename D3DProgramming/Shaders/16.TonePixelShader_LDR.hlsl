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
    float2 distortedUV = uv;
    
    // 1. Scene HDR
    float3 hdr709;

    if (gEnableDistortion != 0)
    {
        // =========================
        // 1. 화면 왜곡
        // =========================
        float2 distortion = GetPoisonDistortion(uv, Time);

        float centerDist = length(uv - 0.5);

        // 중앙 강하고, 가장자리에서 자연스럽게 0
        float centerWeight = smoothstep(0.55, 0.0, centerDist);

        distortedUV += distortion * centerWeight;

        // =========================
        // 2. Color Aberration
        // =========================
        float poisonIntensity = 0.7; // gPoisonIntensity 권장

        // ---- Edge Safety Mask  ----
        float2 safeUV = saturate(distortedUV);

        float edgeMask =
        smoothstep(0.05, 0.15, safeUV.x) *
        smoothstep(0.05, 0.15, safeUV.y) *
        smoothstep(0.05, 0.15, 1.0 - safeUV.x) *
        smoothstep(0.05, 0.15, 1.0 - safeUV.y);

        float aberrationAmount = poisonIntensity * 0.01 * min(centerWeight, edgeMask);

        float2 chromaOffset = normalize(distortion + 1e-4) * aberrationAmount;

        float r = txSceneHDR.Sample(samLinear, safeUV + chromaOffset).r;
        float g = txSceneHDR.Sample(samLinear, safeUV).g;
        float b = txSceneHDR.Sample(samLinear, safeUV - chromaOffset).b;

        hdr709 = float3(r, g, b);
    }
    else
    {
        // 효과 OFF
        hdr709 = txSceneHDR.Sample(samLinear, uv).rgb;
    }
    
    // 2. Exposure (EV)
    float exposureFactor = pow(2.0f, gExposure);
    hdr709 *= exposureFactor;

    // 3. Tone Mapping (ACES)
    float3 tonemapped = ACESFilm(hdr709);

    // 4. 
    float3 pq = LinearToSRGB(tonemapped);

    // 5. R8G8B8A8_UNORM 백버퍼에 출력
    return float4(pq, 1.0);
}