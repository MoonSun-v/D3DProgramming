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
        aiProcess_ConvertToLeftHanded;   // �޼���ǥ�� ��ȯ (DX��)
        // aiProcess_PreTransformVertices;  // ����� ��ȯ����� ������ ���ؽ� �����Ѵ�.  *StaticMesh�� ó���Ҷ���

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
      

    // [ ���̷��� ���� ]
    CreateSkeleton(scene);


    // [ ����޽� �ε� ]
    m_Sections.resize(scene->mNumMeshes);
    for (UINT i = 0; i < m_Sections.size(); ++i)
    {
        m_Sections[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;

        m_Sections[i].m_RefBoneIndex = m_Skeleton[i].m_ParentIndex;


        // SubMesh ��ü ���� ��ȯ �ʱ�ȭ
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


// [ SubMesh ������ Material ���� �� ������ ]
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
            // SubMesh ������
            sub.Render(context, *material, globalCB, pCB, pBoneBuffer, pSampler);
        }
    }
}

// �ִϸ��̼� ������Ʈ 
void SkeletalMesh::Update(float deltaTime)
{
    m_AnimationTime += deltaTime;

    // 1. �� �ִϸ��̼� ��
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

    // 2. SubMesh ��ġ ������Ʈ
    for (auto& section : m_Sections)
    {
        if (section.m_RefBoneIndex >= 0 && section.m_RefBoneIndex < (int)m_Skeleton.size())
        {
            // RefBone �� ��� �������� SubMesh ���� ��� ���ϱ�
            section.m_WorldTransform = m_Skeleton[section.m_RefBoneIndex].m_Model * section.m_WorldTransform;
        }
        else
        {
            // RefBone ������ ��Ʈ ����
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

// [ ���̷��� ���� ]
void SkeletalMesh::CreateSkeleton(const aiScene* scene)
{
    if (!scene->mRootNode) return;

    m_Skeleton.clear(); 

    // Bone ����ü ���� : ��� ��ȸ�ϸ� �� ����
    std::function<void(aiNode*, int, const Matrix&)> traverse =
        [&](aiNode* node, int parentIdx, const Matrix parentTransform)
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

            // bone.m_Model = bone.m_Local;
            // �θ� ��İ� �����ؼ� �������(�����) ���
            bone.m_Model = XMMatrixMultiply(bone.m_Local, parentTransform);

            bone.m_Index = (int)m_Skeleton.size();

            m_Skeleton.push_back(bone);        
            
            // �ڽ� ��� ��� 
            for (UINT i = 0; i < node->mNumChildren; ++i)
                traverse(node->mChildren[i], bone.m_Index, bone.m_Model);

            char buf[256];
            sprintf_s(buf, "%s : ParentIndex=%d , Index=%d\n", bone.m_Name.c_str(), bone.m_ParentIndex, bone.m_Index);
            OutputDebugStringA(buf);
        };

    traverse(scene->mRootNode, -1, DirectX::XMMatrixIdentity());
}