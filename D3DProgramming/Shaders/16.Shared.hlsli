
SamplerState samLinear : register(s0);
SamplerComparisonState samShadow : register(s1); 
SamplerState samLinearIBL : register(s2); // IBL 전용 샘플러 (필요시 설정)
SamplerState samClampIBL : register(s3);  // CLAMP 샘플러 (LUT 샘플용)

Texture2D txBaseColor : register(t0);
Texture2D txNormal : register(t1);
Texture2D txMetallic : register(t2);  
Texture2D txRoughness : register(t3);
Texture2D txEmissive : register(t4);  
Texture2D txOpacity : register(t5);   
Texture2D txShadowMap : register(t6);
Texture2D txSceneHDR : register(t7);

TextureCube txSky : register(t10);
TextureCube txIBL_Diffuse : register(t11);  // Irradiance (Diffuse IBL)
TextureCube txIBL_Specular : register(t12); // Prefiltered Specular Env (mipmaps represent roughness)
Texture2D txIBL_BRDF_LUT : register(t13);   // BRDF LUT (A,B)



float3 ACESFilm(float3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

//--------------------------------------------------------------------------------------
// Constant Buffer Variables  (CPU -> GPU 데이터 전달용)
//--------------------------------------------------------------------------------------

// 1: Rigid, 0: Skinned
cbuffer ConstantBuffer : register(b0)
{
    matrix gWorld;
    matrix gView;
    matrix gProjection;

    float4 vLightDir;
    float4 vLightColor;
    float4 vEyePos;
  
    float4 gMetallicMultiplier;
    float4 gRoughnessMultiplier;
    float4 manualBaseColor;
    
    float gIsRigid;
    float useTexture_BaseColor;
    float useTexture_Metallic;
    float useTexture_Roughness;

    float useTexture_Normal;
    float useIBL;
    float2 pad0;
}


cbuffer BonePoseMatrix : register(b1)
{
    matrix gBonePose[128]; 
}

cbuffer BoneOffsetMatrix : register(b2)
{
    matrix gBoneOffset[128];
}

cbuffer ShadowCB : register(b3)
{
    matrix mWorld;              // 모델 -> 월드
    matrix mLightView;          // 월드 -> 라이트 뷰
    matrix mLightProjection;    // 라이트 뷰 -> 클립
};

cbuffer ToneMapCB : register(b4)
{
    float gExposure;    // EV 기반 exposure
    float gMaxHDRNits; // HDR10 기준 (예: 1000.0)
    float Time;
    float gEnableDistortion;
};


// [ Shadow ]
struct VS_SHADOW_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
    
    uint4 BoneIndices : BLENDINDICES;
    float4 BlendWeights : BLENDWEIGHT0;
};

struct VS_SHADOW_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0; // PS로 전달
};


// [ SkyBox ]
struct VS_INPUT_SKY
{
    float4 Pos : POSITION; 
};

struct PS_INPUT_SKY
{
    float4 Pos : SV_POSITION;     // 변환된 정점 좌표 (화면 클립 공간)
    float3 WorldDir : TEXCOORD0;  // CubeMap 샘플링용
};


// [ Tone Mapping ]
struct VSOut
{
    float4 Pos : SV_POSITION;
    float2 UV : TEXCOORD0;
};


// 정점 입력 구조체
// GPU Input Assembler 단계에서 정점 데이터(Vertex Buffer)와 매핑
struct VS_INPUT
{
    float4 Pos : POSITION;      // 정점 위치
    float3 Norm : NORMAL;       // 정점 법선 벡터
    float2 Tex : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Binormal : BINORMAL;         // CPU에서 미리 지정된 Binormal 추가
    uint4 BoneIndices : BLENDINDICES;   // 4개의 본 인덱스 
    float4 BlendWeights : BLENDWEIGHT0; // 4개의 블렌딩 가중치 
};

// 픽셀 셰이더 입력 구조체 (정점 셰이더 출력 -> 픽셀 셰이더 입력)
struct PS_INPUT
{
    float4 Pos : SV_POSITION;       // 변환된 정점 좌표 (화면 클립 공간)
    float3 Norm : NORMAL;           // 보간된 정점 법선 벡터
    float2 Tex : TEXCOORD0;
    
    float3 WorldPos : TEXCOORD1;    // 월드 공간 위치 전달
    float3 Tangent : TEXCOORD2;     // 월드 공간 Tangent
    float3 Binormal : TEXCOORD3;
    float4 PositionShadow : TEXCOORD4;
};


// [ Hash 함수 ]
//  0~1 사이의 난수 값 반환
//  - dot()으로 좌표를 하나의 값으로 압축
//  - sin()과 큰 상수를 곱해 난수처럼 보이게 변형
//  - frac()으로 소수 부분만 남겨 0~1 범위로 제한
float Hash(float2 p)
{
    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);
}


// [ Noise 함수 ]
//  uv 좌표를 넣으면 부드러운 변화하는 랜덤 값을 반환
//  - UV를 정수 그리드(i)와 그 안의 위치(f)로 분리
//  - 그리드 꼭짓점 4곳에서 Hash 난수값 가져오기 
//  - Smoothstep 보간으로 값의 급격한 변화를 제거
//  - Bilinear interpolation(양선형 보간법)으로 사각형 내부 값을 계산
float Noise(float2 uv)
{
    float2 i = floor(uv); // 현재 좌표의 정수 부분 (그리드 위치)
    float2 f = frac(uv); // 소수 부분 (그리드 내 위치, 0~1)
    
    float a = Hash(i); // 왼쪽 아래
    float b = Hash(i + float2(1, 0)); // 오른쪽 아래
    float c = Hash(i + float2(0, 1)); // 왼쪽 위
    float d = Hash(i + float2(1, 1)); // 오른쪽 위
    
    float2 u = f * f * (3.0 - 2.0 * f); // Smoothstep 보간값 계산

    // bilinear interpolation (사각형 내부를 부드럽게 보간)
    // bilinear interpolation : linear interpolation을 x축과 y축으로 두 번 적용하여 값을 유추하는 방법
    float lerp_x0 = lerp(a, b, u.x); // 아래쪽
    float lerp_x1 = lerp(c, d, u.x); // 위쪽
    
    return lerp(lerp_x0, lerp_x1, u.y); // 위아래 보간 -> 최종 값
}


// [ FBM (Fractional Brownian Motion) : 여러 레이어의 Noise 합성 ]
//  입력 좌표 uv에 대해 여러 주파수/진폭의 노이즈를 합쳐 자연스러운 패턴 생성
float FBM(float2 uv)
{
    float value = 0.0; // 최종 fbm 값
    float amplitude = 0.5; // 초기 진폭
    float frequency = 1.0; // 초기 주파수

    // 여러 옥타브(Noise 레이어)를 합성
    for (int i = 0; i < 4; i++)
    {
        value += amplitude * Noise(uv * frequency); // 각 레이어 Noise 합치기
        frequency *= 2.0; // 다음 레이어는 주파수를 2배로 -> 더 세밀한 패턴
        amplitude *= 0.5; // 진폭은 반으로 줄여서 영향력 감소
    }
    return value;
}

float2 Rotate2D(float2 p, float a)
{
    float s = sin(a);
    float c = cos(a);
    return float2(c * p.x - s * p.y, s * p.x + c * p.y);
}

float2 GetPoisonDistortion(float2 uv, float time)
{
    // 시간 기반 회전 각 (계속 변함)
    float rot = time * 0.7 + FBM(uv * 0.8) * 3.14;

    // UV 자체를 회전시켜 방향성 붕괴
    float2 ruv = Rotate2D(uv - 0.5, rot) + 0.5;

    // 1. 비선형 기본 이동 (방향이 계속 바뀜)
    float2 advectedUV = ruv + float2(
        FBM(ruv * 1.3 + time * 0.4),
        FBM(ruv * 1.3 - time * 0.6)
    ) * 0.15;

    // 2. 큰 흐름 (서로 다른 시간 위상)
    float2 largeWarp;
    largeWarp.x = FBM(advectedUV * 0.7 + time * 0.31);
    largeWarp.y = FBM(advectedUV * 0.9 - time * 0.27);

    // 3. 작은 난류 (시간 + 공간 섞기)
    float2 smallWarp;
    smallWarp.x = FBM(advectedUV * 2.8 + time * 0.11 + largeWarp.y);
    smallWarp.y = FBM(advectedUV * 2.3 - time * 0.14 + largeWarp.x);

    // 4. 2차 Domain Warping (방향성 완전 붕괴)
    float2 warp = largeWarp * 2.0 + smallWarp * 0.9;
    warp = Rotate2D(warp, sin(time * 1.3));

    // 5. 불규칙한 맥동
    float pulse =
        0.6 +
        0.25 * sin(time * 2.1) +
        0.15 * sin(time * 3.7);

    return (warp - 0.5) * 0.085 * pulse;
}

//float2 GetPoisonDistortion(float2 uv, float time)
//{
//    // 화면 전체가 천천히 흐르는 기본 이동
//    float2 advectedUV = uv + float2(
//        FBM(uv * 1.2 + time * 0.6),
//        FBM(uv * 1.2 - time * 0.6)
//    ) * 0.12;

//    // 큰 흐름 (몸이 휘청거리는 느낌)
//    float2 largeWarp;
//    largeWarp.x = FBM(advectedUV * 0.8 + time * 0.2);
//    largeWarp.y = FBM(advectedUV * 0.8 - time * 0.2);

//    // 작은 난류 (시야가 어지러운 느낌)
//    float2 smallWarp;
//    smallWarp.x = FBM(advectedUV * 2.5 + time * 0.1);
//    smallWarp.y = FBM(advectedUV * 2.5 - time * 0.1);

//    // Domain Warping 합성
//    float2 warp = largeWarp * 2.5 + smallWarp * 0.8;
    
//    float pulse = 0.7 + 0.3 * sin(time * 2.0);

//    // 왜곡 강도 조절 
//    return (warp - 0.5) * 0.1 * pulse;
//}