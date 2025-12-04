#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <assimp/mesh.h>
#include "Material.h"
#include "ConstantBuffer.h"
#include "SkeletonInfo.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;


// [ 정점(Vertex) 구조 ]
struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoord;
    XMFLOAT3 Tangent;
    XMFLOAT3 Binormal; 

    // 영향받는 본 수는 최대 4개로 제한한다.
    int BlendIndices[4] = { 0, 0, 0, 0 };               // 참조하는 본의 인덱스의 범위는 최대 128개로 일단 처리함 
    float BlendWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // 가중치 총합은 1이 되어야 한다 

    void AddBoneData(int boneIndex, float weight)
    {
        // 적어도 하나는 데이터가 비어있어야 한다.
        assert(BlendWeights[0] == 0.0f || BlendWeights[1] == 0.0f || BlendWeights[2] == 0.0f || BlendWeights[3] == 0.0f);
        for (int i = 0; i < 4; i++)
        {
            if (BlendWeights[i] == 0.0f)
            {
                BlendIndices[i] = boneIndex;
                BlendWeights[i] = weight;
                return;
            }
        }
    }
};


class SkeletalMeshSection
{
public:
    std::vector<Vertex> Vertices;   // std::vector<Vertex> BoneWeightVertices;
    std::vector<WORD> Indices;      // UNIT 
    
    SkeletonInfo* m_pSkeletonInfo = nullptr;
    
    int m_MaterialIndex = -1;   // 이 서브메시가 참조하는 Material 인덱스
    int m_RefBoneIndex = -1;    // 이 섹션이 종속된 본 인덱스 

    XMMATRIX m_WorldTransform = XMMatrixIdentity(); 

    //  Material SRV 배열 : PBR 텍스처 슬롯
    ComPtr<ID3D11ShaderResourceView> m_SRVs[5]; 


public:
    ComPtr<ID3D11Buffer> m_VertexBuffer;
    ComPtr<ID3D11Buffer> m_IndexBuffer;

    int m_IndexCount = 0;

public:
    // FBX aiMesh -> SubMesh
    void InitializeFromAssimpMesh(ID3D11Device* device, const aiMesh* mesh);
    void Render(ID3D11DeviceContext* context, const Material& mat, ID3D11SamplerState* pSampler);
    void RenderShadow(ID3D11DeviceContext* context, int isRigid);
    void Clear();

private:
    void CreateVertexBuffer(ID3D11Device* device);
    void CreateIndexBuffer(ID3D11Device* device);
    void CreateBoneWeightedVertex(const aiMesh* mesh);
};