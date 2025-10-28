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
        aiProcess_ConvertToLeftHanded;//|  // �޼���ǥ�� ��ȯ (DX��)
        // aiProcess_PreTransformVertices;  // ����� ��ȯ����� ������ ���ؽ� �����Ѵ�.  *SkeletalMesh�� ó���Ҷ���

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
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;

        // rigid ���: �޽ð� � ���� �ٴ��� ���� : �̸����� 
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

		// �޽ð� �����ϴ� ��Ƽ���� �ε��� ����
        // m_MaterialIndex : �޽�(Sections, SubMesh)�� � ��Ƽ����(Material)�� �����ϴ��� ��Ÿ���� �ε���
        // m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;
    }

    // [ ���̷��� ���� ]
    CreateSkeleton(scene);

    return true;
}


// [ SubMesh ������ Material ���� �� ������ ]
void SkeletalMesh::Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11Buffer* pBoneBuffer, ID3D11SamplerState* pSampler)
{
    for (auto& sub : m_Sections)
    {
        ConstantBuffer cb = globalCB;
        // SubMesh�� �� ����� ����
        cb.mWorld = XMMatrixTranspose(sub.m_WorldTransform * m_World);
        // cb.gRefBoneIndex = sub.m_RefBoneIndex; // �� ������ ���ӵ� �� �ε���

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
    m_AnimationTime += deltaTime;

    // �� ������Ʈ 
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

        // �θ�-�ڽ� ���� Ʈ������
        if (bone.m_ParentIndex != -1)
            bone.m_Model = bone.m_Local * m_Skeleton[bone.m_ParentIndex].m_Model;
        else
            bone.m_Model = bone.m_Local;

        //// GPU�� (Transpose�Ͽ� BoneBuffer�� ����)
        //if (bone.m_Index < BoneMatrixContainer::MaxBones)
        //    m_SkeletonPose.SetMatrix(bone.m_Index, XMMatrixTranspose(bone.m_Model));
    }

    // SubMesh�� �� ��ȯ ����
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

// [ ���̷��� ���� ]
void SkeletalMesh::CreateSkeleton(const aiScene* scene)
{
    if (!scene->mRootNode) return;

    // m_SkeletonInfo.CreateFromAiScene(scene);

    m_Skeleton.clear(); 

    // Bone ����ü ���� : ��� ��ȸ�ϸ� �� ����
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
            
            // GPU�� �ʱ�ȭ 
            //if (bone.m_Index < BoneMatrixContainer::MaxBones)
            //    m_SkeletonPose.SetMatrix(bone.m_Index, XMMatrixIdentity());

            // �ڽ� ��� ��� 
            for (UINT i = 0; i < node->mNumChildren; ++i)
                traverse(node->mChildren[i], bone.m_Index);

            char buf[256];
            sprintf_s(buf, "%s : ParentIndex=%d\n", bone.m_Name.c_str(), bone.m_ParentIndex);
            OutputDebugStringA(buf);
        };

    traverse(scene->mRootNode, -1);
}