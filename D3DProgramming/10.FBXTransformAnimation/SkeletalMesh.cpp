#include "SkeletalMesh.h"
#include "Bone.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem> // #include <filesystem>
#include "ConstantBuffer.h"
namespace fs = std::experimental::filesystem;


SkeletalMesh::SkeletalMesh()
{
    m_World = DirectX::XMMatrixIdentity();
}

// [ FBX 파일 로드 ] 
bool SkeletalMesh::LoadFromFBX(ID3D11Device* device, const std::string& path)
{
    Assimp::Importer importer; // 기본 임포트 옵션

    unsigned int importFlags =
        aiProcess_Triangulate |          // 삼각형으로 분할
        aiProcess_GenNormals |           // 노멀 정보 생성
        aiProcess_GenUVCoords |          // 텍스처 좌표 생성
        aiProcess_CalcTangentSpace |     // 탄젠트 벡터 생성 
        aiProcess_ConvertToLeftHanded;   // 왼손좌표계 변환 (DX용)
        // aiProcess_PreTransformVertices;  // 노드의 변환행렬을 적용한 버텍스 생성한다.  *StaticMesh로 처리할때만

    const aiScene* scene = importer.ReadFile(path, importFlags);

    if (!scene) return false;

	// 텍스처 기본 경로
    std::wstring textureBase = fs::path(path).parent_path().wstring();

    // [ 머티리얼 로드 ]
    m_Materials.resize(scene->mNumMaterials);
    for (UINT i = 0; i < scene->mNumMaterials; ++i)
    {
        m_Materials[i].InitializeFromAssimpMaterial(device, scene->mMaterials[i], textureBase);
    }
      

    // [ 스켈레톤 생성 ]
    CreateSkeleton(scene);


    // [ 서브메시 로드 ]
    m_Sections.resize(scene->mNumMeshes);
    for (UINT i = 0; i < m_Sections.size(); ++i)
    {
        m_Sections[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;

        m_Sections[i].m_RefBoneIndex = m_Skeleton[i].m_ParentIndex;


        // SubMesh 자체 로컬 변환 초기화
        m_Sections[i].m_WorldTransform = XMMatrixIdentity();

        char buf[256];
        sprintf_s(buf, " m_MaterialIndex =%d , m_RefBoneIndex=%d \n", m_Sections[i].m_MaterialIndex, m_Sections[i].m_RefBoneIndex);
        OutputDebugStringA(buf);
    }

    //for (UINT i = 0; i < m_Skeleton.size(); ++i)
    //{
    //    char buf[256];
    //    sprintf_s(buf, "[] %s : ParentIndex=%d , Index=%d\n", m_Skeleton[i].m_Name.c_str(), m_Skeleton[i].m_ParentIndex, m_Skeleton[i].m_Index);
    //    OutputDebugStringA(buf);
    //}

    return true;
}


// [ SubMesh 단위로 Material 적용 후 렌더링 ]
void SkeletalMesh::Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11Buffer* pBoneBuffer, ID3D11SamplerState* pSampler)
{
    for (auto& sub : m_Sections)
    {
        ConstantBuffer cb = globalCB;
        cb.mWorld = XMMatrixTranspose(m_World * sub.m_WorldTransform);

        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Materials.size())
        {
            material = &m_Materials[sub.m_MaterialIndex];
        }

        if (material)
        {
            // SubMesh 렌더링
            sub.Render(context, *material, globalCB, pCB, pBoneBuffer, pSampler);
        }
    }
}

// 애니메이션 업데이트 
void SkeletalMesh::Update(float deltaTime)
{
    m_AnimationTime += deltaTime;

    // 1. 본 애니메이션 평가
    for (auto& bone : m_Skeleton)
    {
        if (bone.m_pBoneAnimation)
        {
            Vector3 pos, scale;
            Quaternion rot;
            bone.m_pBoneAnimation->Evaluate(m_AnimationTime, pos, rot, scale);

            bone.m_Local = Matrix::CreateScale(scale) *
                Matrix::CreateFromQuaternion(rot) *
                Matrix::CreateTranslation(pos);
        }

        if (bone.m_ParentIndex != -1)
        {
            bone.m_Model = XMMatrixMultiply(bone.m_Local, m_Skeleton[bone.m_ParentIndex].m_Model);
        }
        else
        {
            bone.m_Model = bone.m_Local;
        }
    }

    // 2. SubMesh 위치 업데이트
    for (auto& section : m_Sections)
    {
        if (section.m_RefBoneIndex >= 0 && section.m_RefBoneIndex < (int)m_Skeleton.size())
        {
            // RefBone 모델 행렬 기준으로 SubMesh 로컬 행렬 곱하기
            section.m_WorldTransform = m_Skeleton[section.m_RefBoneIndex].m_Model * section.m_WorldTransform;
        }
        else
        {
            // RefBone 없으면 루트 기준
            section.m_WorldTransform = m_Skeleton[0].m_Model * section.m_WorldTransform;
        }
    }
}


void SkeletalMesh::Clear()
{
    // Section Clear  // SubMesh Clear
    for (auto& sub : m_Sections) sub.Clear();
    m_Sections.clear();

    // Material Clear
    for (auto& mat : m_Materials) mat.Clear();
    m_Materials.clear();

    m_Skeleton.clear();
    // m_SkeletonPose.Clear();
    m_Animations.clear();
}

// [ 스켈레톤 생성 ]
void SkeletalMesh::CreateSkeleton(const aiScene* scene)
{
    if (!scene->mRootNode) return;

    m_Skeleton.clear(); 

    // Bone 구조체 생성 : 노드 순회하며 본 생성
    std::function<void(aiNode*, int, const Matrix&)> traverse =
        [&](aiNode* node, int parentIdx, const Matrix parentTransform)
        {
            Bone bone;
            bone.m_Name = node->mName.C_Str();
            bone.m_ParentIndex = parentIdx;

            // 변환행렬 복사
            aiMatrix4x4 transform = node->mTransformation;
            bone.m_Local = Matrix(
                transform.a1, transform.b1, transform.c1, transform.d1,
                transform.a2, transform.b2, transform.c2, transform.d2,
                transform.a3, transform.b3, transform.c3, transform.d3,
                transform.a4, transform.b4, transform.c4, transform.d4);

            // bone.m_Model = bone.m_Local;
            // 부모 행렬과 결합해서 월드행렬(모델행렬) 계산
            bone.m_Model = XMMatrixMultiply(bone.m_Local, parentTransform);

            bone.m_Index = (int)m_Skeleton.size();

            m_Skeleton.push_back(bone);        
            
            // 자식 노드 재귀 
            for (UINT i = 0; i < node->mNumChildren; ++i)
                traverse(node->mChildren[i], bone.m_Index, bone.m_Model);

            char buf[256];
            sprintf_s(buf, "%s : ParentIndex=%d , Index=%d\n", bone.m_Name.c_str(), bone.m_ParentIndex, bone.m_Index);
            OutputDebugStringA(buf);
        };

    traverse(scene->mRootNode, -1, DirectX::XMMatrixIdentity());
}