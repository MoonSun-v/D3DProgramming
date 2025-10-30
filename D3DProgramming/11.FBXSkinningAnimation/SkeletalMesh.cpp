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
        // aiProcess_PreTransformVertices;  // ����� ��ȯ����� ������ ���ؽ� �����Ѵ�.  **StaticMesh�� ó���Ҷ��� ��� 

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

    
    // [ �ִϸ��̼� ���� ���� ]
    bool hasAnimation = (scene->mNumAnimations > 0);
    OutputDebugString((L"[hasAnimation] " + std::to_wstring(hasAnimation) + L"\n").c_str());

    // �ִϸ��̼��� ������ Animation ����Ʈ ä���
    if (hasAnimation)
    {
        m_Animations.resize(scene->mNumAnimations);

        for (UINT animIdx = 0; animIdx < scene->mNumAnimations; ++animIdx)
        {
            aiAnimation* aiAnim = scene->mAnimations[animIdx];
            Animation& anim = m_Animations[animIdx];

            anim.Name = aiAnim->mName.C_Str();
            anim.Duration = static_cast<float>(aiAnim->mDuration / aiAnim->mTicksPerSecond);

            // BoneAnimation ä���
            anim.BoneAnimations.resize(m_Skeleton.size()); // Skeleton�� �� ������ŭ ����

            for (UINT c = 0; c < aiAnim->mNumChannels; ++c)
            {
                aiNodeAnim* channel = aiAnim->mChannels[c];
                std::string boneName = channel->mNodeName.C_Str();

                // Skeleton���� �̸����� �ε��� ã��
                auto it = std::find_if(m_Skeleton.begin(), m_Skeleton.end(), [&](const Bone& b) { return b.m_Name == boneName; });

                if (it != m_Skeleton.end())
                {
                    int boneIndex = static_cast<int>(it - m_Skeleton.begin());
                    BoneAnimation& boneAnim = anim.BoneAnimations[boneIndex];

                    // ��ġ Ű
                    for (UINT k = 0; k < channel->mNumPositionKeys; ++k)
                    {
                        aiVectorKey& pk = channel->mPositionKeys[k];
                        AnimationKey key;
                        key.Time = static_cast<float>(pk.mTime / aiAnim->mTicksPerSecond);
                        key.Position = Vector3(pk.mValue.x, pk.mValue.y, pk.mValue.z);
                        boneAnim.AnimationKeys.push_back(key);
                    }

                    // ȸ�� Ű
                    for (UINT k = 0; k < channel->mNumRotationKeys; ++k)
                    {
                        aiQuatKey& rk = channel->mRotationKeys[k];
                        AnimationKey& key = boneAnim.AnimationKeys[k]; // ���� �ε��� ���
                        key.Rotation = Quaternion(rk.mValue.x, rk.mValue.y, rk.mValue.z, rk.mValue.w);
                    }

                    // ������ Ű
                    for (UINT k = 0; k < channel->mNumScalingKeys; ++k)
                    {
                        aiVectorKey& sk = channel->mScalingKeys[k];
                        AnimationKey& key = boneAnim.AnimationKeys[k]; // ���� �ε��� ���
                        key.Scaling = Vector3(sk.mValue.x, sk.mValue.y, sk.mValue.z);
                    }

                    // Skeleton�� ���� �� BoneAnimation ����
                    it->m_pBoneAnimation = &boneAnim;
                }
            }
        }
    }

    
    // [ ����޽� �ε� ]
    m_Sections.resize(scene->mNumMeshes);
    OutputDebugString((L"[m_Sections.size()] " + std::to_wstring(m_Sections.size()) + L"\n").c_str());

    for (UINT i = 0; i < m_Sections.size(); ++i)
    {
        m_Sections[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;

        FindMeshBoneMapping(scene->mRootNode, scene);
    }

    return true;
}

// [ �Ž� - �� ���� ]
void SkeletalMesh::FindMeshBoneMapping(aiNode* node, const aiScene* scene)
{
    // 1. ���� ��� �̸��� ��ġ�ϴ� �� �ε��� ã��
    int boneIndex = -1;
    for (int i = 0; i < (int)m_Skeleton.size(); ++i)
    {
        if (_stricmp(m_Skeleton[i].m_Name.c_str(), node->mName.C_Str()) == 0)
        {
            boneIndex = i;
            break;
        }
    }

    // 2. �� ��尡 ���� Mesh�鿡 ���� RefBoneIndex ����
    for (UINT i = 0; i < node->mNumMeshes; ++i)
    {
        UINT meshIndex = node->mMeshes[i];
        if (meshIndex < m_Sections.size())
        {
            m_Sections[meshIndex].m_RefBoneIndex = boneIndex;

            // [ Mesh - Bone ¦�� ����� ]
            //char buf[256];
            //sprintf_s(buf, "[MeshMapping] Mesh[%d] �� Bone[%d] (%s)\n", meshIndex, boneIndex, m_Skeleton[boneIndex].m_Name.c_str());
            //OutputDebugStringA(buf);
        }
    }

    // 3. �ڽ� ��嵵 ��������� ó��
    for (UINT i = 0; i < node->mNumChildren; ++i)
    {
        FindMeshBoneMapping(node->mChildren[i], scene);
    }
}


// [ SubMesh ������ Material ���� �� ������ ]
void SkeletalMesh::Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11Buffer* pBoneBuffer, ID3D11SamplerState* pSampler)
{
    for (auto& sub : m_Sections)
    {
        ConstantBuffer cb = globalCB;

        int refBone = sub.m_RefBoneIndex;  // OutputDebugString((L"[refBone] " + std::to_wstring(refBone) + L"\n").c_str());

        if (refBone != -1)
        {
            cb.mWorld = XMMatrixTranspose(m_Skeleton[refBone].m_Model * sub.m_WorldTransform);
        }
        else
        {
            cb.mWorld = XMMatrixTranspose(sub.m_WorldTransform);
        }

        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Materials.size())
        {
            material = &m_Materials[sub.m_MaterialIndex];
        }

        if (material)
        {
            sub.Render(context, *material, cb, pCB, pBoneBuffer, pSampler); // SubMesh ������
        }
    }
}

void SkeletalMesh::Update(float deltaTime, const Matrix& worldTransform)
{
    if (m_Animations.empty()) return;

    Animation& anim = m_Animations[m_AnimationIndex];
    m_AnimationTime += deltaTime;
    if (m_AnimationTime > anim.Duration)
    {
        m_AnimationTime = fmod(m_AnimationTime, anim.Duration);
    }

    // SimpleMath::Matrix -> XMMATRIX
    XMMATRIX worldMat = XMLoadFloat4x4(&worldTransform);

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
            bone.m_Model = bone.m_Local * m_Skeleton[bone.m_ParentIndex].m_Model; 
        }
        else
        {
            bone.m_Model = bone.m_Local * worldMat; // ���� : ��Ʈ ���� ����  
        }

        // [ m_Model ����� ��� ]
        //XMFLOAT4X4 m;
        //XMStoreFloat4x4(&m, bone.m_Model);
        //char buf[512];
        //sprintf_s( buf, "%s m_Model:\n" "[%f %f %f]\n", bone.m_Name.c_str(), m._41, m._42, m._43 );
        //OutputDebugStringA(buf);
    }
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

            // ��ȯ��� ���� : �� -> �� : ��ġ�� ���·� �� 
            aiMatrix4x4 transform = node->mTransformation;
            bone.m_Local = Matrix(
                transform.a1, transform.b1, transform.c1, transform.d1,
                transform.a2, transform.b2, transform.c2, transform.d2,
                transform.a3, transform.b3, transform.c3, transform.d3,
                transform.a4, transform.b4, transform.c4, transform.d4
            );
        
            // �θ� ��İ� �����ؼ� �������(�����) ���
            bone.m_Model = XMMatrixMultiply(bone.m_Local, parentTransform);

            bone.m_Index = (int)m_Skeleton.size();

            m_Skeleton.push_back(bone);        
            
            // �ڽ� ��� ��� 
            for (UINT i = 0; i < node->mNumChildren; ++i)
            {
                traverse(node->mChildren[i], bone.m_Index, bone.m_Model);
            }

            char buf[256];
            sprintf_s(buf, "%s : ParentIndex=%d , Index=%d\n", bone.m_Name.c_str(), bone.m_ParentIndex, bone.m_Index);
            OutputDebugStringA(buf);
        };

    traverse(scene->mRootNode, -1, DirectX::XMMatrixIdentity());
}


void SkeletalMesh::Clear()
{
    for (auto& sub : m_Sections) sub.Clear();
    m_Sections.clear();

    for (auto& mat : m_Materials) mat.Clear();
    m_Materials.clear();

    m_Skeleton.clear();
    m_Animations.clear();
}