#pragma once
#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <DirectXMath.h>
#include <vector>
#include <assimp/mesh.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;


// [ ���� ���� ]
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
    std::vector<WORD> Indices;

    UINT m_VertextBufferStride = 0;					// ���ؽ� �ϳ��� ũ�� (����Ʈ ����)
    UINT m_VertextBufferOffset = 0;					// ���ؽ� ������ ������

private:
    ComPtr<ID3D11Buffer> m_VertexBuffer;
    ComPtr<ID3D11Buffer> m_IndexBuffer;

    int m_IndexCount = 0;

public:
    void InitializeFromAssimpMesh(ID3D11Device* device, const aiMesh* mesh);
    void Render(ID3D11DeviceContext* context);
};