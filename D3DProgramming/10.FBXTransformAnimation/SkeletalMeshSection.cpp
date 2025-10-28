#include "SkeletalMeshSection.h"
#include "ConstantBuffer.h"
#include "../Common/D3DDevice.h"

// Assimp FBX aiMesh -> SkeletalMeshSection �ʱ�ȭ
void SkeletalMeshSection::InitializeFromAssimpMesh(ID3D11Device* device, const aiMesh* mesh)
{
	// [ ���ؽ� ������ ���� ]
    Vertices.resize(mesh->mNumVertices);
    for (UINT i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertices[i].Position = XMFLOAT3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

        Vertices[i].Normal = XMFLOAT3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);

        if (mesh->HasTextureCoords(0))
            Vertices[i].TexCoord = XMFLOAT2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

        if (mesh->HasTangentsAndBitangents())
        {
            Vertices[i].Tangent = XMFLOAT3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            Vertices[i].Binormal = XMFLOAT3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
    }

    // [2] �� ����ġ ������ ����
    // CreateBoneWeightedVertex(mesh);

    // [ �ε��� ������ ���� ]
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        for (UINT j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
        {
            Indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }
    m_IndexCount = (int)Indices.size();
    // m_MaterialIndex = mesh->mMaterialIndex;  // �޽ð� �����ϴ� ��Ƽ���� �ε���

    // [3] �ε��� ���� ����
    CreateIndexBuffer(device);

    // [4] GPU ���ؽ� ���� ����
    CreateVertexBuffer(device);

    // [5] ��Ƽ���� / �� ���� ����
    // m_MaterialIndex = mesh->mMaterialIndex;
    // SetSkeletonInfo(mesh);
}

// [���ؽ� ���� ����]
void SkeletalMeshSection::CreateVertexBuffer(ID3D11Device* device)
{
    if (Vertices.empty()) return;

    // Position / Normal / UV / Tangent / Binormal ���� ����
    D3D11_BUFFER_DESC bd{};
    D3D11_SUBRESOURCE_DATA init{};

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * (UINT)Vertices.size();
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    init.pSysMem = Vertices.data();
    device->CreateBuffer(&bd, &init, m_VertexBuffer.GetAddressOf());
}


// [�ε��� ���� ����]
void SkeletalMeshSection::CreateIndexBuffer(ID3D11Device* device)
{
    // IndexBuffer
    D3D11_BUFFER_DESC bd{};
    D3D11_SUBRESOURCE_DATA init{};

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * (UINT)Indices.size();
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    init.pSysMem = Indices.data();
    device->CreateBuffer(&bd, &init, m_IndexBuffer.GetAddressOf());
}


// [ SubMesh ������ ������ ]
void SkeletalMeshSection::Render(
    ID3D11DeviceContext* context,
    const Material& mat,
    const ConstantBuffer& cb,
    ID3D11Buffer* pConstantBuffer,
    ID3D11Buffer* pBoneBuffer,
    ID3D11SamplerState* pSampler)    
{
    // Vertex / Index ���ε�
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0); // DXGI_FORMAT_R32_UINT

    // ������� ���ε�
    context->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
    context->VSSetConstantBuffers(0, 1, &pConstantBuffer);
    context->PSSetConstantBuffers(0, 1, &pConstantBuffer);

    // Material �ؽ�ó ���ε�
    const TextureSRVs& tex = mat.GetTextures();
    ID3D11ShaderResourceView* srvs[5] =
    {
        tex.DiffuseSRV.Get(),
        tex.NormalSRV.Get(),
        tex.SpecularSRV.Get(),
        tex.EmissiveSRV.Get(),
        tex.OpacitySRV.Get()
    };
    context->PSSetShaderResources(0, 5, srvs);
    context->PSSetSamplers(0, 1, &pSampler);

    // �׸���
    context->DrawIndexed(m_IndexCount, 0, 0);
}

void SkeletalMeshSection::Clear()
{
    m_VertexBuffer.Reset();
    m_IndexBuffer.Reset();

    Vertices.clear();
    // BoneWeightVertices.clear();
    Indices.clear();
    m_IndexCount = 0;
    m_MaterialIndex = -1;
    m_RefBoneIndex = -1;
    m_WorldTransform = XMMatrixIdentity();
}