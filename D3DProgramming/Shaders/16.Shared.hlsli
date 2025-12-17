
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
// Texture2D txAO : register(t14);          // Ambient Occlusion map


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
    float Exposure;
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
