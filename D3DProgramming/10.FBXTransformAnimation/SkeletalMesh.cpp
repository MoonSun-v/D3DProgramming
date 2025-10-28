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

// [ FBX ���� �ε� ] 
bool SkeletalMesh::LoadFromFBX(ID3D11Device* device, const std::string& path)
{
    Assimp::Importer importer; // �⺻ ����Ʈ �ɼ�

    unsigned int importFlags =
        aiProcess_Triangulate |          // �ﰢ������ ����
        aiProcess_GenNormals |           // ��� ���� ����
        aiProcess_GenUVCoords |          // �ؽ�ó ��ǥ ����
        aiProcess_CalcTangentSpace |     // ź��Ʈ ���� ���� 
        aiProcess_ConvertToLeftHanded |  // �޼���ǥ�� ��ȯ (DX��)
        aiProcess_PreTransformVertices;  // ����� ��ȯ����� ������ ���ؽ� �����Ѵ�.  *SkeletalMesh�� ó���Ҷ���

    const aiScene* scene = importer.ReadFile(path, importFlags);

    if (!scene) return false;

	// �ؽ�ó �⺻ ���
    std::wstring textureBase = fs::path(path).parent_path().wstring();

    // [ ��Ƽ���� �ε� ]
    m_Materials.resize(scene->mNumMaterials);
    for (UINT i = 0; i < scene->mNumMaterials; ++i)
    {
        m_Materials[i].InitializeFromAssimpMaterial(device, scene->mMaterials[i], textureBase);
    }
        
    // [ ����޽� �ε� ]
    m_Sections.resize(scene->mNumMeshes);
    for (UINT i = 0; i < scene->mNumMeshes; ++i)
    {
        m_Sections[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);

		// �޽ð� �����ϴ� ��Ƽ���� �ε��� ����
        // m_MaterialIndex : �޽�(Sections, SubMesh)�� � ��Ƽ����(Material)�� �����ϴ��� ��Ÿ���� �ε���
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;
    }

    // [���̷��� ����]
    CreateSkeleton(scene);

    return true;
}


// [ SubMesh ������ Material ���� �� ������ ]
void SkeletalMesh::Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11Buffer* pBoneBuffer, ID3D11SamplerState* pSampler)
{
    for (auto& sub : m_Sections)
    {
        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Materials.size())
            material = &m_Materials[sub.m_MaterialIndex];

        if (!material) continue;

        // SubMesh ������
        sub.Render(context, *material, globalCB, pCB, pBoneBuffer, pSampler);
    }
}

// �ִϸ��̼� ������Ʈ 
void SkeletalMesh::Update(float deltaTime)
{
    if (/*m_Animations.empty() ||*/ m_Skeleton.empty())
        return;

    //const Animation& anim = m_Animations[m_AnimationIndex];
    //m_AnimationProgressTime += deltaTime;
    //m_AnimationProgressTime = fmod(m_AnimationProgressTime, anim.Duration);

    // ���� Ʈ������ ���
    for (auto& bone : m_Skeleton)
    {
        if (bone.m_pBoneAnimation)
        {
            Vector3 position, scaling;
            Quaternion rotation;

            bone.m_pBoneAnimation->Evaluate(m_AnimationProgressTime, position, rotation, scaling);

            bone.m_Local =
                Matrix::CreateScale(scaling) *
                Matrix::CreateFromQuaternion(rotation) *
                Matrix::CreateTranslation(position);
        }

        // �θ� ���� ����
        if (bone.m_ParentIndex != -1)
            bone.m_Model = bone.m_Local * m_Skeleton[bone.m_ParentIndex].m_Model;
        else
            bone.m_Model = bone.m_Local;

        // GPU�� �� ��� ����
        if (bone.m_Index < BoneMatrixContainer::MaxBones)
            m_SkeletonPose.SetMatrix(bone.m_Index, XMMatrixTranspose(bone.m_Model));
    }
}

void SkeletalMesh::UpdateBoneBuffer(ID3D11DeviceContext* context, ID3D11Buffer* pBoneBuffer)
{
    context->UpdateSubresource(pBoneBuffer, 0, nullptr, &m_SkeletonPose.m_Model, 0, 0);
    context->VSSetConstantBuffers(1, 1, &pBoneBuffer);
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
    m_SkeletonPose.Clear();
    m_Animations.clear();
}

// [���̷��� ����]
void SkeletalMesh::CreateSkeleton(const aiScene* scene)
{
    if (!scene->mRootNode) return;

    m_SkeletonPose.Clear(); // ���� Pose �ʱ�ȭ

    // ��� ��ȸ�ϸ� �� ����
    std::function<void(aiNode*, int)> traverse = [&](aiNode* node, int parentIdx)
        {
            Bone bone;
            bone.m_Name = node->mName.C_Str();
            bone.m_ParentIndex = parentIdx;

            // ��ȯ��� ����
            aiMatrix4x4 transform = node->mTransformation;
            bone.m_Local = Matrix(
                transform.a1, transform.b1, transform.c1, transform.d1,
                transform.a2, transform.b2, transform.c2, transform.d2,
                transform.a3, transform.b3, transform.c3, transform.d3,
                transform.a4, transform.b4, transform.c4, transform.d4);

            bone.m_Model = bone.m_Local;
            bone.m_Index = (int)m_Skeleton.size();

            m_Skeleton.push_back(bone);        
            
            if (bone.m_Index < BoneMatrixContainer::MaxBones)
                m_SkeletonPose.SetMatrix(bone.m_Index, XMMatrixIdentity());

            for (UINT i = 0; i < node->mNumChildren; ++i)
                traverse(node->mChildren[i], bone.m_Index);
        };

    traverse(scene->mRootNode, -1);
}