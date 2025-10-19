#include "StaticMeshSection.h"
#include "ConstantBuffer.h"
#include "D3DDevice.h"

void StaticMeshSection::InitializeFromAssimpMesh(ID3D11Device* device, const aiMesh* mesh)
{
	// [ 버텍스 데이터 추출 ]
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

	// [ 인덱스 데이터 추출 ]
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        // const aiFace& face = mesh->mFaces[i];
        for (UINT j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
        {
            Indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }
    // m_IndexCount = (UINT)Indices.size();
    m_IndexCount = (int)Indices.size();

	m_MaterialIndex = mesh->mMaterialIndex;  // 메시가 참조하는 머티리얼 인덱스



	// [ 버텍스 버퍼 & 인덱스 버퍼 생성 ]

    D3D11_BUFFER_DESC bd{};
    D3D11_SUBRESOURCE_DATA init{};

    // 버텍스
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * (UINT)Vertices.size();
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    // bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU가 읽고 쓰는 기본 버퍼
    // bd.CPUAccessFlags = 0;

    init.pSysMem = Vertices.data();
    device->CreateBuffer(&bd, &init, m_VertexBuffer.GetAddressOf());


    // 인덱스
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * (UINT)Indices.size();
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    init.pSysMem = Indices.data();
    device->CreateBuffer(&bd, &init, m_IndexBuffer.GetAddressOf());
}

// TODO : 머티리얼 적용 버전 
void StaticMeshSection::Render(
    ID3D11DeviceContext* context,
    const Material& mat,
    const ConstantBuffer& cb,
    ID3D11Buffer* pConstantBuffer,
    ID3D11SamplerState* pSampler)    
{
    // Vertex / Index 바인딩
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    // 상수버퍼 업로드
    context->UpdateSubresource(pConstantBuffer, 0, nullptr, &cb, 0, 0);
    context->VSSetConstantBuffers(0, 1, &pConstantBuffer);
    context->PSSetConstantBuffers(0, 1, &pConstantBuffer);

    // Material 텍스처 SRV 바인딩
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

    // 그리기
    context->DrawIndexed(m_IndexCount, 0, 0);
}
