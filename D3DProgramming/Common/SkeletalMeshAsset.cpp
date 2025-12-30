#include "SkeletalMeshAsset.h"
#include "../Common/D3DDevice.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem> // #include <filesystem>
#include "ConstantBuffer.h"
namespace fs = std::experimental::filesystem;

// [ FBX 파일 로드 ] 
bool SkeletalMeshAsset::LoadFromFBX(ID3D11Device* device, const std::string& path)
{
    // [ 본 오프셋용 상수 버퍼 : b2 (Offset) ]
    D3D11_BUFFER_DESC bdBone = {};
    bdBone.Usage = D3D11_USAGE_DEFAULT;
    bdBone.ByteWidth = sizeof(BoneMatrixContainer);
    bdBone.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bdBone.CPUAccessFlags = 0;
    HR_T(device->CreateBuffer(&bdBone, nullptr, m_pBoneOffsetBuffer.GetAddressOf()));


    Assimp::Importer importer; // 기본 임포트 옵션

    unsigned int importFlags =
        aiProcess_Triangulate |          // 삼각형으로 분할
        aiProcess_GenNormals |           // 노멀 정보 생성
        aiProcess_GenUVCoords |          // 텍스처 좌표 생성
        aiProcess_CalcTangentSpace |     // 탄젠트 벡터 생성 
        aiProcess_LimitBoneWeights |     // 본의 영향을 받는 정점의 최대 개수 
        aiProcess_ConvertToLeftHanded;   // 왼손좌표계 변환 (DX용) 

    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false); // $_AssimpFbx$ 노드 제거 
    const aiScene* scene = importer.ReadFile(path, importFlags);

    if (!scene) return false;

    std::wstring textureBase = fs::path(path).parent_path().wstring(); // 텍스처 기본 경로

    bool hasBones = false;
    for (UINT meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
    {
        if (scene->mMeshes[meshIdx]->HasBones())
        {
            hasBones = true;
            break;
        }
    }

    if (!hasBones)
        importFlags |= aiProcess_PreTransformVertices;

    // [ 머티리얼 로드 ]
    m_Materials.resize(scene->mNumMaterials);
    for (UINT i = 0; i < scene->mNumMaterials; ++i)
    {
        m_Materials[i].InitializeFromAssimpMaterial(device, scene->mMaterials[i], textureBase);
    }

    if (hasBones)
    {
        // [ 스켈레톤 정보 생성 ]
        m_pSkeletonInfo = std::make_unique<SkeletonInfo>();
        m_pSkeletonInfo->CreateFromAiScene(scene);

        // [ 본 트리 생성 ]
        CreateSkeleton(scene);
    }
    else
    {
        m_pSkeletonInfo = nullptr; // StaticMesh
    }


    // [ 애니메이션 ]
    bool hasAnimation = (scene->mNumAnimations > 0);
    OutputDebugString((L"[hasAnimation] " + std::to_wstring(hasAnimation) + L"\n").c_str());

    if (hasAnimation)
    {
        m_Animations.resize(scene->mNumAnimations);

        for (UINT animIdx = 0; animIdx < scene->mNumAnimations; ++animIdx)
        {
            aiAnimation* aiAnim = scene->mAnimations[animIdx];
            AnimationClip& anim = m_Animations[animIdx];

            anim.Name = aiAnim->mName.length > 0 ? aiAnim->mName.C_Str() : ("Anim_" + std::to_string(animIdx));
            anim.Duration = float(aiAnim->mDuration / aiAnim->mTicksPerSecond);
            
            // BoneAnimation 채우기
            anim.BoneAnimations.resize(m_Skeleton.size());

            for (UINT c = 0; c < aiAnim->mNumChannels; ++c)
            {
                aiNodeAnim* channel = aiAnim->mChannels[c];
                std::string boneName = channel->mNodeName.C_Str();

                // Skeleton에서 이름으로 인덱스 찾기
                auto it = std::find_if( m_Skeleton.begin(), m_Skeleton.end(), [&](const Bone& b) { return b.m_Name == boneName; });

                if (it == m_Skeleton.end())
                    continue;

                int boneIndex = static_cast<int>(it - m_Skeleton.begin());
                BoneAnimation& boneAnim = anim.BoneAnimations[boneIndex];

                // 위치 키
                for (UINT k = 0; k < channel->mNumPositionKeys; ++k)
                {
                    aiVectorKey& pk = channel->mPositionKeys[k];
                    boneAnim.Positions.push_back({
                        float(pk.mTime / aiAnim->mTicksPerSecond),
                        Vector3(pk.mValue.x, pk.mValue.y, pk.mValue.z)
                        });
                }

                // 회전 키
                for (UINT k = 0; k < channel->mNumRotationKeys; ++k)
                {
                    aiQuatKey& rk = channel->mRotationKeys[k];
                    boneAnim.Rotations.push_back({
                        float(rk.mTime / aiAnim->mTicksPerSecond),
                        Quaternion(rk.mValue.x, rk.mValue.y, rk.mValue.z, rk.mValue.w)
                        });
                }

                // 스케일 키
                for (UINT k = 0; k < channel->mNumScalingKeys; ++k)
                {
                    aiVectorKey& sk = channel->mScalingKeys[k];
                    boneAnim.Scales.push_back({
                        float(sk.mTime / aiAnim->mTicksPerSecond),
                        Vector3(sk.mValue.x, sk.mValue.y, sk.mValue.z)
                        });
                }

                // 기존에는 Skeleton의 본에 이 BoneAnimation 연결 했음.. 
            }
        }
    }

    // [ 서브메시 로드 ]
    m_Sections.resize(scene->mNumMeshes);
    OutputDebugString((L"[m_Sections.size()] " + std::to_wstring(m_Sections.size()) + L"\n").c_str());

    for (UINT i = 0; i < m_Sections.size(); ++i)
    {
        // Skinned이면 skeleton 공유, static이면 nullptr
        m_Sections[i].m_pSkeletonInfo = m_pSkeletonInfo.get();
        m_Sections[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;
    }

    return true;
}

// [ FBX에서 애니메이션 로드 ]
bool SkeletalMeshAsset::LoadAnimationFromFBX(const std::string& path, const std::string& overrideName)
{
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_ConvertToLeftHanded);

    if (!scene || scene->mNumAnimations == 0)
        return false;

    aiAnimation* aiAnim = scene->mAnimations[0];

    AnimationClip anim;
    anim.Name = overrideName.empty() ? fs::path(path).stem().string() : overrideName;

    anim.Duration = float(aiAnim->mDuration / aiAnim->mTicksPerSecond);
    anim.BoneAnimations.resize(m_Skeleton.size());

    for (UINT c = 0; c < aiAnim->mNumChannels; ++c)
    {
        aiNodeAnim* channel = aiAnim->mChannels[c];
        std::string boneName = channel->mNodeName.C_Str();

        auto it = std::find_if(m_Skeleton.begin(), m_Skeleton.end(), [&](const Bone& b) { return b.m_Name == boneName; });

        if (it == m_Skeleton.end())
            continue;

        int boneIndex = int(it - m_Skeleton.begin());
        BoneAnimation& boneAnim = anim.BoneAnimations[boneIndex];

        for (UINT k = 0; k < channel->mNumPositionKeys; ++k)
        {
            auto& pk = channel->mPositionKeys[k];
            boneAnim.Positions.push_back({
                float(pk.mTime / aiAnim->mTicksPerSecond),
                Vector3(pk.mValue.x, pk.mValue.y, pk.mValue.z)
                });
        }

        for (UINT k = 0; k < channel->mNumRotationKeys; ++k)
        {
            auto& rk = channel->mRotationKeys[k];
            boneAnim.Rotations.push_back({
                float(rk.mTime / aiAnim->mTicksPerSecond),
                Quaternion(rk.mValue.x, rk.mValue.y, rk.mValue.z, rk.mValue.w)
                });
        }

        for (UINT k = 0; k < channel->mNumScalingKeys; ++k)
        {
            auto& sk = channel->mScalingKeys[k];
            boneAnim.Scales.push_back({
                float(sk.mTime / aiAnim->mTicksPerSecond),
                Vector3(sk.mValue.x, sk.mValue.y, sk.mValue.z)
                });
        }
    }

    m_Animations.push_back(std::move(anim));
    return true;
}


int SkeletalMeshAsset::FindAnimationIndexByName(const std::string& name) const
{
    for (size_t i = 0; i < m_Animations.size(); ++i)
    {
        if (m_Animations[i].Name == name)
            return static_cast<int>(i);
    }
    return -1;
}

const AnimationClip* SkeletalMeshAsset::GetAnimation(int index) const
{
    if (index < 0 || index >= (int)m_Animations.size())
        return nullptr;

    return &m_Animations[index];
}

void SkeletalMeshAsset::CreateSkeleton(const aiScene* scene)
{
    if (!scene->mRootNode) return;

    bool hasBones = false;
    for (UINT meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
        if (scene->mMeshes[meshIdx]->HasBones()) { hasBones = true; break; }

    if (!hasBones) return; // StaticMesh인 경우 건너뜀

    m_Skeleton.clear();

    // 1. Bone 이름 수집
    std::unordered_set<std::string> boneNameSet;
    for (UINT meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
    {
        aiMesh* mesh = scene->mMeshes[meshIdx];
        for (UINT i = 0; i < mesh->mNumBones; ++i)
        {
            boneNameSet.insert(mesh->mBones[i]->mName.C_Str());
        }
    }

    // 2. Bone Local (BindLocal) 생성
    std::function<void(aiNode*, int, const Matrix&)> traverse =
        [&](aiNode* node, int parentIdx, const Matrix parentTransform)
        {
            std::string nodeName = node->mName.C_Str();
            bool isRealBone = boneNameSet.count(nodeName) > 0;
            int thisBoneIndex = parentIdx;

            if (isRealBone)
            {
                Bone bone;
                bone.m_Name = nodeName;
                bone.m_ParentIndex = parentIdx;

                // ［부모 기준 T-pose 로컬 트랜스폼］　aiNode Transform -> Bind Pose
                aiMatrix4x4 t = node->mTransformation;
                bone.m_BindLocal = Matrix(
                    t.a1, t.b1, t.c1, t.d1,
                    t.a2, t.b2, t.c2, t.d2,
                    t.a3, t.b3, t.c3, t.d3,
                    t.a4, t.b4, t.c4, t.d4
                );

                bone.m_Local = bone.m_BindLocal;    // 초기 T-Pose
                bone.m_Model = bone.m_Local;        // Model은 Update에서 계산

                //aiMatrix4x4 transform = node->mTransformation;
                //bone.m_Local = Matrix(
                //    transform.a1, transform.b1, transform.c1, transform.d1,
                //    transform.a2, transform.b2, transform.c2, transform.d2,
                //    transform.a3, transform.b3, transform.c3, transform.d3,
                //    transform.a4, transform.b4, transform.c4, transform.d4
                //);

                //bone.m_Model = bone.m_Local; // 바인드 포즈에서는 누적 X 

                bone.m_Index = (int)m_Skeleton.size();
                thisBoneIndex = bone.m_Index;
                m_Skeleton.push_back(bone);
            }

            for (UINT i = 0; i < node->mNumChildren; ++i)
            {
                if (isRealBone)
                {
                    traverse(node->mChildren[i], thisBoneIndex, m_Skeleton[thisBoneIndex].m_Model);
                }
                else
                {
                    traverse(node->mChildren[i], thisBoneIndex, parentTransform);
                }
            }
        };

    traverse(scene->mRootNode, -1, DirectX::XMMatrixIdentity());
}

void SkeletalMeshAsset::Clear()
{
    // Bone offset buffer
    if (m_pBoneOffsetBuffer) m_pBoneOffsetBuffer.Reset();

    // 각 Section에서 D3D 버퍼 해제
    for (auto& section : m_Sections)
    {
        if (section.m_VertexBuffer) section.m_VertexBuffer.Reset();
        if (section.m_IndexBuffer) section.m_IndexBuffer.Reset();

        for (int i = 0; i < 6; i++)
        {
            if (section.m_SRVs[i]) section.m_SRVs[i].Reset();
        }
    }

    m_Sections.clear();
    m_Materials.clear();
    m_Animations.clear();
    m_Skeleton.clear();
    m_pSkeletonInfo.reset();
}