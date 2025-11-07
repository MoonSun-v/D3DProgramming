#include "SkeletalMeshSection.h"
#include "ConstantBuffer.h"
#include "../Common/D3DDevice.h"
#include "SkeletonInfo.h"

// Assimp FBX aiMesh -> SkeletalMeshSection 초기화
void SkeletalMeshSection::InitializeFromAssimpMesh(ID3D11Device* device, const aiMesh* mesh)
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

    // [ 본 가중치 데이터 생성 ]
    CreateBoneWeightedVertex(mesh);

    // [ 인덱스 데이터 추출 ]
    for (UINT i = 0; i < mesh->mNumFaces; ++i)
    {
        for (UINT j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
        {
            Indices.push_back(mesh->mFaces[i].mIndices[j]);
        }
    }
    m_IndexCount = (int)Indices.size();
    m_MaterialIndex = mesh->mMaterialIndex;  // 메시가 참조하는 머티리얼 인덱스

    // 인덱스 정보 생성
    CreateIndexBuffer(device);

    // GPU 버텍스 버퍼 생성
    CreateVertexBuffer(device);
}

// [ 버텍스 버퍼 생성 ]
void SkeletalMeshSection::CreateVertexBuffer(ID3D11Device* device)
{
    if (Vertices.empty()) return;

    D3D11_BUFFER_DESC bd{};
    D3D11_SUBRESOURCE_DATA init{};

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(Vertex) * (UINT)Vertices.size();
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

    init.pSysMem = Vertices.data();
    device->CreateBuffer(&bd, &init, m_VertexBuffer.GetAddressOf());
}


// [ 인덱스 버퍼 생성 ]
void SkeletalMeshSection::CreateIndexBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC bd{};
    D3D11_SUBRESOURCE_DATA init{};

    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = sizeof(WORD) * (UINT)Indices.size();
    bd.BindFlags = D3D11_BIND_INDEX_BUFFER;

    init.pSysMem = Indices.data();
    device->CreateBuffer(&bd, &init, m_IndexBuffer.GetAddressOf());
}

// [ 본 가중치 정보 생성 ]
void SkeletalMeshSection::CreateBoneWeightedVertex(const aiMesh* mesh)
{
    if (!m_pSkeletonInfo) return;

    for (UINT i = 0; i < mesh->mNumBones; ++i)
    {
        aiBone* pAiBone = mesh->mBones[i];
        std::string boneName = pAiBone->mName.C_Str();

        int boneIndex = m_pSkeletonInfo->GetBoneIndexByBoneName(boneName.c_str());

        if (boneIndex == -1) // 매칭 실패 
        {
            char buf[256];
            sprintf_s(buf, "[Mesh:%s] Bone '%s' -> NOT FOUND in SkeletonInfo\n", mesh->mName.C_Str(), boneName.c_str());
            OutputDebugStringA(buf);
            continue;
        }

        // [ OffsetMatrix ] SkeletonInfo의 BoneOffsetMatrices에 저장 -> 여기에서 Transpose() 함 
        m_pSkeletonInfo->BoneOffsetMatrices.SetMatrix(boneIndex, Matrix(&pAiBone->mOffsetMatrix.a1)); 

        // 각 버텍스에 본 가중치 적용
        for (UINT j = 0; j < pAiBone->mNumWeights; ++j)
        {
            UINT vertexId = pAiBone->mWeights[j].mVertexId;
            float weight = pAiBone->mWeights[j].mWeight;

            Vertices[vertexId].AddBoneData(boneIndex, weight);
        }
    }
}

void SkeletalMeshSection::Render(ID3D11DeviceContext* context, const Material& mat, ID3D11SamplerState* pSampler)    
{
    // Vertex / Index 바인딩
    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    context->IASetVertexBuffers(0, 1, m_VertexBuffer.GetAddressOf(), &stride, &offset);
    context->IASetIndexBuffer(m_IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0); // DXGI_FORMAT_R32_UINT

    // Material 텍스처 바인딩
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

void SkeletalMeshSection::Clear()
{
    m_VertexBuffer.Reset();
    m_IndexBuffer.Reset();

    Vertices.clear();
    Indices.clear();
    m_IndexCount = 0;
    m_MaterialIndex = -1;
    m_RefBoneIndex = -1;
    m_WorldTransform = XMMatrixIdentity();
}