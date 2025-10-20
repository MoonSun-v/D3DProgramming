#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <assimp/mesh.h>
#include "Material.h"
#include "ConstantBuffer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// [ 정점 선언 ]
struct Vertex
{
    XMFLOAT3 Position;
    XMFLOAT3 Normal;
    XMFLOAT2 TexCoord;
    XMFLOAT3 Tangent;
    XMFLOAT3 Binormal;
};

class StaticMeshSection
{
public:
    std::vector<Vertex> Vertices;
    std::vector<WORD> Indices; //UNIT 

    int m_MaterialIndex = -1;   // 이 서브메시가 참조하는 머티리얼 인덱스

private:
    ComPtr<ID3D11Buffer> m_VertexBuffer;
    ComPtr<ID3D11Buffer> m_IndexBuffer;

    int m_IndexCount = 0;

public:
    void InitializeFromAssimpMesh(ID3D11Device* device, const aiMesh* mesh);
    void Render(
        ID3D11DeviceContext* context,
        const Material& mat,
        const ConstantBuffer& globalCB,
        ID3D11Buffer* pConstantBuffer,
        ID3D11SamplerState* pSampler);
};