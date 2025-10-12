
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);
Texture2D txNormal : register(t1); 
Texture2D texSpecular : register(t2);

//--------------------------------------------------------------------------------------
// Constant Buffer Variables  (CPU -> GPU ������ ���޿�)
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix World; // ���� ��ȯ ��� (�� ��ǥ �� ���� ��ǥ)
    matrix View; // �� ��ȯ ��� (���� ��ǥ �� ī�޶� ��ǥ)
    matrix Projection; // ���� ��ȯ ��� (ī�޶� ��ǥ �� Ŭ�� ��ǥ)

    float4 vLightDir; // ���� ���� ���� 
    float4 vLightColor; // ���� ���� 
    float4 vOutputColor; // �ܻ� �������� ��� ����
    
    float4 vEyePos; // ī�޶� ��ġ
    float4 vAmbient; // ��Ƽ���� Ambient
    float4 vDiffuse; // ��Ƽ���� Diffuse
    float4 vSpecular; // ��Ƽ���� Specular
    float fShininess; // ��¦�� ����
    
    float pad[3]; // 16����Ʈ ���� �е�
}


// ���� �Է� ����ü
// - GPU Input Assembler �ܰ迡�� ���� ������(Vertex Buffer)�� ����
struct VS_INPUT
{
    float4 Pos : POSITION;      // ���� ��ġ
    float3 Norm : NORMAL;       // ���� ���� ����
    float2 Tex : TEXCOORD0;
    float3 Tangent : TANGENT;
};

// �ȼ� ���̴� �Է� ����ü (���� ���̴� ��� -> �ȼ� ���̴� �Է�)
struct PS_INPUT
{
    float4 Pos : SV_POSITION;   // ��ȯ�� ���� ��ǥ (ȭ�� Ŭ�� ����)
    float3 Norm : NORMAL;       // ������ ���� ���� ����
    float2 Tex : TEXCOORD0;
    
    float3 WorldPos : TEXCOORD1; // ���� ���� ��ġ ����
    float3 Tangent : TEXCOORD2; // ���� ���� Tangent
    // float3 Bitangent : TEXCOORD3; // ���� ���� Bitangent
};
