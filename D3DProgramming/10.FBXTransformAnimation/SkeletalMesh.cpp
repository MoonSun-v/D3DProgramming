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
        aiProcess_ConvertToLeftHanded;//|  // 왼손좌표계 변환 (DX용)
        // aiProcess_PreTransformVertices;  // 노드의 변환행렬을 적용한 버텍스 생성한다.  *SkeletalMesh로 처리할때만

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

      
    // [ 서브메시 로드 ]
    m_Sections.resize(scene->mNumMeshes);
    for (UINT i = 0; i < scene->mNumMeshes; ++i)
    {
        m_Sections[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;

        // rigid 방식: 메시가 어떤 본에 붙는지 저장 : 이름별로 
        if (scene->mMeshes[i]->mNumBones > 0)
        {
            std::string boneName = scene->mMeshes[i]->mBones[0]->mName.C_Str();
            auto it = std::find_if(m_Skeleton.begin(), m_Skeleton.end(),
                [&](const Bone& b) { return b.m_Name == boneName; });
            m_Sections[i].m_RefBoneIndex = (it != m_Skeleton.end()) ? it->m_Index : -1;
        }
        else
        {
            m_Sections[i].m_RefBoneIndex = -1;
        }

		// 메시가 참조하는 머티리얼 인덱스 설정
        // m_MaterialIndex : 메시(Sections, SubMesh)가 어떤 머티리얼(Material)을 참조하는지 나타내는 인덱스
        // m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;
    }

    // [ 스켈레톤 생성 ]
    CreateSkeleton(scene);

    return true;
}


// [ SubMesh 단위로 Material 적용 후 렌더링 ]
void SkeletalMesh::Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11Buffer* pBoneBuffer, ID3D11SamplerState* pSampler)
{
    for (auto& sub : m_Sections)
    {
        ConstantBuffer cb = globalCB;
        // SubMesh의 본 행렬을 적용
        cb.mWorld = XMMatrixTranspose(sub.m_WorldTransform * m_World);
        // cb.gRefBoneIndex = sub.m_RefBoneIndex; // 이 섹션이 종속된 본 인덱스

        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Materials.size())
            material = &m_Materials[sub.m_MaterialIndex];

        if (!material) continue;

        // SubMesh 렌더링
        sub.Render(context, *material, globalCB, pCB, pBoneBuffer, pSampler);
    }
}

// 애니메이션 업데이트 
void SkeletalMesh::Update(float deltaTime)
{
    m_AnimationTime += deltaTime;

    // 본 업데이트 
    for (auto& bone : m_Skeleton)
    {
        if (bone.m_pBoneAnimation)
        {
            Vector3 position, scaling;
            Quaternion rotation;
            bone.m_pBoneAnimation->Evaluate(m_AnimationTime, position, rotation, scaling);

            bone.m_Local =
                Matrix::CreateScale(scaling) *
                Matrix::CreateFromQuaternion(rotation) *
                Matrix::CreateTranslation(position);
        }

        // 부모-자식 누적 트랜스폼
        if (bone.m_ParentIndex != -1)
            bone.m_Model = bone.m_Local * m_Skeleton[bone.m_ParentIndex].m_Model;
        else
            bone.m_Model = bone.m_Local;

        //// GPU용 (Transpose하여 BoneBuffer에 복사)
        //if (bone.m_Index < BoneMatrixContainer::MaxBones)
        //    m_SkeletonPose.SetMatrix(bone.m_Index, XMMatrixTranspose(bone.m_Model));
    }

    // SubMesh에 본 변환 적용
    for (auto& sub : m_Sections)
    {
        if (sub.m_RefBoneIndex >= 0 && sub.m_RefBoneIndex < (int)m_Skeleton.size())
            sub.m_WorldTransform = m_Skeleton[sub.m_RefBoneIndex].m_Model;
        else
            sub.m_WorldTransform = XMMatrixIdentity();
    }
}

//void SkeletalMesh::UpdateBoneBuffer(ID3D11DeviceContext* context, ID3D11Buffer* pBoneBuffer)
//{
//    context->UpdateSubresource(pBoneBuffer, 0, nullptr, &m_SkeletonPose, 0, 0);
//    context->VSSetConstantBuffers(1, 1, &pBoneBuffer);
//}


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

    // m_SkeletonInfo.CreateFromAiScene(scene);

    m_Skeleton.clear(); 

    // Bone 구조체 생성 : 노드 순회하며 본 생성
    std::function<void(aiNode*, int)> traverse = [&](aiNode* node, int parentIdx)
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

            bone.m_Model = bone.m_Local;
            bone.m_Index = (int)m_Skeleton.size();

            m_Skeleton.push_back(bone);        
            
            // GPU용 초기화 
            //if (bone.m_Index < BoneMatrixContainer::MaxBones)
            //    m_SkeletonPose.SetMatrix(bone.m_Index, XMMatrixIdentity());

            // 자식 노드 재귀 
            for (UINT i = 0; i < node->mNumChildren; ++i)
                traverse(node->mChildren[i], bone.m_Index);

            char buf[256];
            sprintf_s(buf, "%s : ParentIndex=%d\n", bone.m_Name.c_str(), bone.m_ParentIndex);
            OutputDebugStringA(buf);
        };

    traverse(scene->mRootNode, -1);
}