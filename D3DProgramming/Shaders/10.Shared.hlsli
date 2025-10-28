
Texture2D txDiffuse : register(t0);
SamplerState samLinear : register(s0);
Texture2D txNormal : register(t1);
Texture2D txSpecular : register(t2);
Texture2D txEmissive : register(t3);
Texture2D txOpacity : register(t4);

//--------------------------------------------------------------------------------------
// Constant Buffer Variables  (CPU -> GPU ������ ���޿�)
//--------------------------------------------------------------------------------------
cbuffer ConstantBuffer : register(b0)
{
    matrix gWorld; // ���� ��ȯ ��� (�� ��ǥ �� ���� ��ǥ)
    matrix gView; // �� ��ȯ ��� (���� ��ǥ �� ī�޶� ��ǥ)
    matrix gProjection; // ���� ��ȯ ��� (ī�޶� ��ǥ �� Ŭ�� ��ǥ)

    float4 vLightDir; // ���� ���� ���� 
    float4 vLightColor; // ���� ���� 
    float4 vOutputColor; // �ܻ� �������� ��� ����
    float4 vEyePos; // ī�޶� ��ġ
    
    float4 vAmbient; // ��Ƽ���� Ambient
    float4 vDiffuse; // ��Ƽ���� DiffuseA
    float4 vSpecular; // ��Ƽ���� Specular
    
    float fShininess; // ��¦�� ����
    float pad1[3]; // 16����Ʈ ���� �е�
    
    float gIsRigid; // 1: Rigid, 0: Skinned
    float gRefBoneIndex; // �������� �� ���� �� �ε���
    float pad2[2]; // 16����Ʈ ����
    
    float pad3[4];
    float pad4[4];
}


// �� �� ��ȯ ��� 
cbuffer ModelMatrix : register(b1)
{
    matrix gModelMatricies[128]; 
}


// ���� �Է� ����ü
// GPU Input Assembler �ܰ迡�� ���� ������(Vertex Buffer)�� ����
struct VS_INPUT
{
    float4 Pos : POSITION;      // ���� ��ġ
    float3 Norm : NORMAL;       // ���� ���� ����
    float2 Tex : TEXCOORD0;
    float3 Tangent : TANGENT;
    float3 Binormal : BINORMAL; // CPU���� �̸� ������ Binormal �߰�
    
    // uint BoneIndex : BLENDINDICES; // �� �ε��� 
    // float4 BoneWeights : BLENDWEIGHT0;
};


// �ȼ� ���̴� �Է� ����ü (���� ���̴� ��� -> �ȼ� ���̴� �Է�)
struct PS_INPUT
{
    float4 Pos : SV_POSITION;       // ��ȯ�� ���� ��ǥ (ȭ�� Ŭ�� ����)
    float3 Norm : NORMAL;           // ������ ���� ���� ����
    float2 Tex : TEXCOORD0;
    
    float3 WorldPos : TEXCOORD1;    // ���� ���� ��ġ ����
    float3 Tangent : TEXCOORD2;     // ���� ���� Tangent
    float3 Binormal : TEXCOORD3;
};
