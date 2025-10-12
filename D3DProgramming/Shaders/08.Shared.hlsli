
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);
Texture2D txNormal : register(t1); 
Texture2D texSpecular : register(t2);

//--------------------------------------------------------------------------------------
// Constant Buffer Variables  (CPU -> GPU 데이터 전달용)
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix World; // 월드 변환 행렬 (모델 좌표 → 월드 좌표)
    matrix View; // 뷰 변환 행렬 (월드 좌표 → 카메라 좌표)
    matrix Projection; // 투영 변환 행렬 (카메라 좌표 → 클립 좌표)

    float4 vLightDir; // 광원 방향 벡터 
    float4 vLightColor; // 광원 색상 
    float4 vOutputColor; // 단색 렌더링용 출력 색상
    
    float4 vEyePos; // 카메라 위치
    float4 vAmbient; // 머티리얼 Ambient
    float4 vDiffuse; // 머티리얼 Diffuse
    float4 vSpecular; // 머티리얼 Specular
    float fShininess; // 반짝임 정도
    
    float pad[3]; // 16바이트 정렬 패딩
}


// 정점 입력 구조체
// - GPU Input Assembler 단계에서 정점 데이터(Vertex Buffer)와 매핑
struct VS_INPUT
{
    float4 Pos : POSITION;      // 정점 위치
    float3 Norm : NORMAL;       // 정점 법선 벡터
    float2 Tex : TEXCOORD0;
    float3 Tangent : TANGENT;
};

// 픽셀 셰이더 입력 구조체 (정점 셰이더 출력 -> 픽셀 셰이더 입력)
struct PS_INPUT
{
    float4 Pos : SV_POSITION;   // 변환된 정점 좌표 (화면 클립 공간)
    float3 Norm : NORMAL;       // 보간된 정점 법선 벡터
    float2 Tex : TEXCOORD0;
    
    float3 WorldPos : TEXCOORD1; // 월드 공간 위치 전달
    float3 Tangent : TEXCOORD2; // 월드 공간 Tangent
    // float3 Bitangent : TEXCOORD3; // 월드 공간 Bitangent
};
