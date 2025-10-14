#include "StaticMesh.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem> // #include <filesystem>
namespace fs = std::experimental::filesystem;


StaticMesh::StaticMesh()
{
    m_World = DirectX::XMMatrixIdentity();
}

// [ FBX 파일 로드 ] 
bool StaticMesh::LoadFromFBX(ID3D11Device* device, const std::string& path)
{
    Assimp::Importer importer; // 기본 임포트 옵션

    unsigned int importFlags =
        aiProcess_Triangulate |          // 삼각형으로 분할
        aiProcess_GenNormals |           // 노멀 정보 생성
        aiProcess_GenUVCoords |          // 텍스처 좌표 생성
        aiProcess_CalcTangentSpace |     // 탄젠트 벡터 생성 
        aiProcess_ConvertToLeftHanded |  // 왼손좌표계 변환 (DX용)
        aiProcess_PreTransformVertices;  // 노드의 변환행렬을 적용한 버텍스 생성한다.  *StaticMesh로 처리할때만

    const aiScene* scene = importer.ReadFile(path, importFlags);

    if (!scene) return false;

	// 텍스처 경로
    std::wstring textureBase = fs::path(path).parent_path().wstring();

    // [ 머티리얼 로드 ]
    m_Materials.resize(scene->mNumMaterials);
    for (UINT i = 0; i < scene->mNumMaterials; ++i)
    {
        m_Materials[i].InitializeFromAssimpMaterial(device, scene->mMaterials[i], textureBase);
    }
        
    // [ 서브메시 로드 ]
    m_SubMeshes.resize(scene->mNumMeshes);
    for (UINT i = 0; i < scene->mNumMeshes; ++i)
    {
        m_SubMeshes[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);

		// 메시가 참조하는 머티리얼 인덱스 설정
        // m_MaterialIndex : 메시(SubMesh)가 어떤 머티리얼(Material)을 참조하는지 나타내는 인덱스
        m_SubMeshes[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;
    }

    return true;
}

void StaticMesh::Render(ID3D11DeviceContext* context)
{
    for (auto& sub : m_SubMeshes)
    {
        sub.Render(context);
    }    
}

// TODO : 머티리얼 적용
//void StaticMesh::Render(ID3D11DeviceContext* context)
//{
//    for (auto& sub : m_SubMeshes)
//    {
//        Material* material = nullptr;
//        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Materials.size())
//            material = &m_Materials[sub.m_MaterialIndex];
//
//        sub.Render(context, material);
//    }
//}


// [ Assimp 파일 로드 ]
//const aiScene* StaticMesh::ReadFile(const std::string& path, Assimp::Importer& importer)
//{
//    unsigned int importFlags =
//        aiProcess_Triangulate |          // 삼각형으로 분할
//        aiProcess_GenNormals |           // 노멀 생성
//        aiProcess_GenUVCoords |          // 텍스처 좌표 생성
//        aiProcess_CalcTangentSpace |     // 탄젠트 계산
//        aiProcess_ConvertToLeftHanded |  // 왼손좌표계 변환 (DX용)
//        aiProcess_PreTransformVertices;  // 노드 변환 적용 (StaticMesh용)
//
//    const aiScene* scene = importer.ReadFile(path, importFlags);
//    return scene;
//}