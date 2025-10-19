#include "StaticMesh.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem> // #include <filesystem>
#include "ConstantBuffer.h"
namespace fs = std::experimental::filesystem;


StaticMesh::StaticMesh()
{
    m_World = DirectX::XMMatrixIdentity();
}

// [ FBX ���� �ε� ] 
bool StaticMesh::LoadFromFBX(ID3D11Device* device, const std::string& path)
{
    Assimp::Importer importer; // �⺻ ����Ʈ �ɼ�

    unsigned int importFlags =
        aiProcess_Triangulate |          // �ﰢ������ ����
        aiProcess_GenNormals |           // ��� ���� ����
        aiProcess_GenUVCoords |          // �ؽ�ó ��ǥ ����
        aiProcess_CalcTangentSpace |     // ź��Ʈ ���� ���� 
        aiProcess_ConvertToLeftHanded |  // �޼���ǥ�� ��ȯ (DX��)
        aiProcess_PreTransformVertices;  // ����� ��ȯ����� ������ ���ؽ� �����Ѵ�.  *StaticMesh�� ó���Ҷ���

    const aiScene* scene = importer.ReadFile(path, importFlags);

    if (!scene) return false;

	// �ؽ�ó ���
    std::wstring textureBase = fs::path(path).parent_path().wstring();

    // [ ��Ƽ���� �ε� ]
    m_Materials.resize(scene->mNumMaterials);
    for (UINT i = 0; i < scene->mNumMaterials; ++i)
    {
        m_Materials[i].InitializeFromAssimpMaterial(device, scene->mMaterials[i], textureBase);
    }
        
    // [ ����޽� �ε� ]
    m_SubMeshes.resize(scene->mNumMeshes);
    for (UINT i = 0; i < scene->mNumMeshes; ++i)
    {
        m_SubMeshes[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);

		// �޽ð� �����ϴ� ��Ƽ���� �ε��� ����
        // m_MaterialIndex : �޽�(SubMesh)�� � ��Ƽ����(Material)�� �����ϴ��� ��Ÿ���� �ε���
        m_SubMeshes[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;
    }

    return true;
}

// TODO : ��Ƽ���� ����
void StaticMesh::Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11SamplerState* pSampler)
{
    for (auto& sub : m_SubMeshes)
    {
        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Materials.size())
            material = &m_Materials[sub.m_MaterialIndex];

        if (material)
            sub.Render(context, *material, globalCB, pCB, pSampler);
    }
}
