#include "17.Shared.hlsli"

//--------------------------------------------------------------------------------------
// Pixel Shader (HDR 출력용)
//--------------------------------------------------------------------------------------

// ============================================
// PQ 인코딩에 사용되는 상수 (ST.2084) 
// ============================================
static const float PQ_m1 = 2610.0 / 4096.0 / 4.0; // 0.1593017578125
static const float PQ_m2 = 2523.0 / 4096.0 * 128.0; // 78.84375
static const float PQ_c1 = 3424.0 / 4096.0; // 0.8359375
static const float PQ_c2 = 2413.0 / 4096.0 * 32.0; // 18.8515625
static const float PQ_c3 = 2392.0 / 4096.0 * 32.0; // 18.6875

// Rec.709 -> Rec.2020
static const float3x3 Rec709to2020 =
{
    0.6274040f, 0.3292820f, 0.0433136f,
    0.0690970f, 0.9195400f, 0.0113612f,
    0.0163916f, 0.0880132f, 0.8955950f
};

float3 LinearToPQ(float3 linear709, float maxNits)
{
    // 1. Rec.709 -> Rec.2020 변환 (옵션, Unreal에서도 비슷한 매트릭스 사용)
    float3 color2020 = mul(Rec709to2020, linear709);

    // tonemapped linear 값을 maxNits로 스케일링 후 PQ 적용
    
    // 2. Nits 정규화 (HDR10 기준 10,000 nits)
    float3 norm = color2020 * (maxNits / 10000.0f);

    // 3. PQ encoding
    float3 powered = pow(abs(norm), PQ_m1);
    float3 num = PQ_c1 + PQ_c2 * powered;
    float3 den = 1.0f + PQ_c3 * powered;

    return pow(num / den, PQ_m2);
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

    // 4. PQ로 변환 (HDR10 출력용)
    float3 pq = LinearToPQ(tonemapped, gMaxHDRNits);

    // 5. 최종 PQ 인코딩된 값 [0.0, 1.0]을 R10G10B10A2_UNORM 백버퍼에 출력
    return float4(pq, 1.0);
}