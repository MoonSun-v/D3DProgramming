
SamplerState samLinear : register(s0);
SamplerComparisonState samShadow : register(s1); 

Texture2D txBaseColor : register(t0);
Texture2D txNormal : register(t1);
Texture2D txMetallic : register(t2);  // Texture2D txSpecular : register(t2);
Texture2D txRoughness : register(t3); // Texture2D txEmissive : register(t3);

Texture2D txEmissive : register(t4);  // Texture2D txOpacity : register(t4); 
Texture2D txOpacity : register(t5);   // Texture2D txShadowMap : register(t5);
Texture2D txShadowMap : register(t6);


//--------------------------------------------------------------------------------------
// Constant Buffer Variables  (CPU -> GPU 데이터 전달용)
//--------------------------------------------------------------------------------------
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
    
    int gIsRigid; // 1: Rigid, 0: Skinned
    float pad[3];
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


// [ Shadow ]
struct VS_SHADOW_INPUT
{
    float4 Pos : POSITION;
    float2 Tex : TEXCOORD0;
    
    // 스키닝용
    uint4 BoneIndices : BLENDINDICES;
    float4 BlendWeights : BLENDWEIGHT0;
};

struct VS_SHADOW_OUTPUT
{
    float4 Pos : SV_POSITION;
    float2 Tex : TEXCOORD0; // PS로 전달
};


// 정점 입력 구조체
// GPU Input Assembler 단계에서 정점 데이터(Vertex Buffer)와 매핑
struct VS_INPUT
{
    float4 Pos : POSITION;      // 정점 위치
    float3 Norm : NORMAL;       // 정점 법선 벡터
    float2 Tex : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Binormal : BINORMAL; // CPU에서 미리 지정된 Binormal 추가
    uint4 BoneIndices : BLENDINDICES; // 4개의 본 인덱스 
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
