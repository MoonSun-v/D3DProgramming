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
        aiProcess_LimitBoneWeights |     // 본의 영향을 받는 정점의 최대 개수 
        aiProcess_ConvertToLeftHanded;   // 왼손좌표계 변환 (DX용)
        // aiProcess_PreTransformVertices;  // 노드의 변환행렬을 적용한 버텍스 생성한다.  **StaticMesh로 처리할때만 사용 

    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false); // $_AssimpFbx$ 노드 제거 

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

    // [ 스켈레톤 정보 생성 ]
    m_pSkeletonInfo = std::make_unique<SkeletonInfo>();
    m_pSkeletonInfo->CreateFromAiScene(scene);

    // [ 본 트리 생성 ]
    CreateSkeleton(scene);

    // [ 애니메이션 로드 ]
    bool hasAnimation = (scene->mNumAnimations > 0);
    OutputDebugString((L"[hasAnimation] " + std::to_wstring(hasAnimation) + L"\n").c_str());

    if (hasAnimation) // 애니메이션이 있으면 Animation 리스트 채우기
    {
        m_Animations.resize(scene->mNumAnimations);

        for (UINT animIdx = 0; animIdx < scene->mNumAnimations; ++animIdx)
        {
            aiAnimation* aiAnim = scene->mAnimations[animIdx];
            Animation& anim = m_Animations[animIdx];

            anim.Name = aiAnim->mName.C_Str();
            anim.Duration = static_cast<float>(aiAnim->mDuration / aiAnim->mTicksPerSecond);

            // BoneAnimation 채우기
            anim.BoneAnimations.resize(m_Skeleton.size()); // Skeleton의 본 개수만큼 생성
            // const auto& bones = m_pSkeletonInfo->Bones;
            // anim.BoneAnimations.resize(bones.size());

            for (UINT c = 0; c < aiAnim->mNumChannels; ++c)
            {
                aiNodeAnim* channel = aiAnim->mChannels[c];
                std::string boneName = channel->mNodeName.C_Str();

                // Skeleton에서 이름으로 인덱스 찾기
                auto it = std::find_if(m_Skeleton.begin(), m_Skeleton.end(), [&](const Bone& b) { return b.m_Name == boneName; });

                if (it != m_Skeleton.end())
                {
                    int boneIndex = static_cast<int>(it - m_Skeleton.begin());
                    BoneAnimation& boneAnim = anim.BoneAnimations[boneIndex];

                    UINT maxKeys = ((( channel->mNumPositionKeys) > (channel->mNumRotationKeys)) ? ( channel->mNumPositionKeys) : (channel->mNumRotationKeys));
                    boneAnim.AnimationKeys.resize(maxKeys); // 안전하게 resize

                    // 위치 키
                    for (UINT k = 0; k < channel->mNumPositionKeys; ++k)
                    {
                        aiVectorKey& pk = channel->mPositionKeys[k];
                        AnimationKey key;
                        key.Time = static_cast<float>(pk.mTime / aiAnim->mTicksPerSecond);
                        key.Position = Vector3(pk.mValue.x, pk.mValue.y, pk.mValue.z);
                        boneAnim.AnimationKeys.push_back(key);
                    }

                    // 회전 키
                    for (UINT k = 0; k < channel->mNumRotationKeys; ++k)
                    {
                        aiQuatKey& rk = channel->mRotationKeys[k];
                        AnimationKey& key = boneAnim.AnimationKeys[k]; // 같은 인덱스 사용
                        key.Rotation = Quaternion(rk.mValue.x, rk.mValue.y, rk.mValue.z, rk.mValue.w);
                    }

                    // 스케일 키
                    for (UINT k = 0; k < channel->mNumScalingKeys; ++k)
                    {
                        aiVectorKey& sk = channel->mScalingKeys[k];
                        AnimationKey& key = boneAnim.AnimationKeys[k]; // 같은 인덱스 사용
                        key.Scaling = Vector3(sk.mValue.x, sk.mValue.y, sk.mValue.z);
                    }

                    // Skeleton의 본에 이 BoneAnimation 연결
                    it->m_pBoneAnimation = &boneAnim;
                }
            }
        }
    } 

    // [ 서브메시 로드 ]
    m_Sections.resize(scene->mNumMeshes);
    OutputDebugString((L"[m_Sections.size()] " + std::to_wstring(m_Sections.size()) + L"\n").c_str());

    for (UINT i = 0; i < m_Sections.size(); ++i)
    {
        m_Sections[i].m_pSkeletonInfo = m_pSkeletonInfo.get(); // Skinned : 모든 매시가 같은 스켈레톤 공유 
        m_Sections[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;
    }
    
    return true;
}

void SkeletalMesh::CreateSkeleton(const aiScene* scene)
{
    if (!scene->mRootNode) return;

    m_Skeleton.clear(); 

    std::unordered_set<std::string> boneNameSet;
    for (UINT meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
    {
        aiMesh* mesh = scene->mMeshes[meshIdx];
        for (UINT i = 0; i < mesh->mNumBones; ++i)
        {
            boneNameSet.insert(mesh->mBones[i]->mName.C_Str());
        }
    }

    std::function<void(aiNode*, int, const Matrix&)> traverse =
        [&](aiNode* node, int parentIdx, const Matrix parentTransform)
        {
            std::string nodeName = node->mName.C_Str();

            bool isRealBone = boneNameSet.count(nodeName) > 0;

            int thisBoneIndex = parentIdx;

            Bone bone;

            if (isRealBone)
            {
                bone.m_Name = nodeName;
                bone.m_ParentIndex = parentIdx;

                aiMatrix4x4 transform = node->mTransformation;
                bone.m_Local = Matrix(
                    transform.a1, transform.b1, transform.c1, transform.d1,
                    transform.a2, transform.b2, transform.c2, transform.d2,
                    transform.a3, transform.b3, transform.c3, transform.d3,
                    transform.a4, transform.b4, transform.c4, transform.d4
                );

                bone.m_Model = XMMatrixMultiply(bone.m_Local, parentTransform);

                bone.m_Index = (int)m_Skeleton.size();
                thisBoneIndex = bone.m_Index;

                m_Skeleton.push_back(bone);

                char buf[256];
                sprintf_s(buf, "%s : ParentIndex=%d , Index=%d\n", bone.m_Name.c_str(), bone.m_ParentIndex, bone.m_Index);
                OutputDebugStringA(buf);
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


void SkeletalMesh::Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11Buffer* pBonePoseBuffer,
    ID3D11Buffer* pBoneOffsetBuffer, ID3D11SamplerState* pSampler)
{
    for (auto& sub : m_Sections)
    {
        ConstantBuffer cb = globalCB;

        cb.mWorld = XMMatrixIdentity();

        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Materials.size())
        {
            material = &m_Materials[sub.m_MaterialIndex];
        }

        if (material)
        {
            sub.Render(context, *material, cb, pCB, pBonePoseBuffer, pBoneOffsetBuffer, pSampler); // SubMesh 렌더링
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

    // OutputDebugStringA((std::string("m_Skeleton.size() = ") + std::to_string(m_Skeleton.size()) + "\n").c_str());
    
    // 본 포즈 계산 
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

        //if (bone.m_ParentIndex != -1)
        //{
        //    bone.m_Model = bone.m_Local * m_Skeleton[bone.m_ParentIndex].m_Model;
        //}
        //else 
        //{
        //    bone.m_Model = bone.m_Local * worldMat; // 조정 : 루트 본만 적용  
        //}

        if (bone.m_ParentIndex != -1)
        {
            bone.m_Model = m_Skeleton[bone.m_ParentIndex].m_Model * bone.m_Local;
        }
        else
        {
            bone.m_Model = bone.m_Local; // * worldMat; // 조정 : 루트 본만 적용  
        }
    }

    // GPU용 본 행렬 계산
    m_SkeletonPose.SetBoneCount((int)m_Skeleton.size());

    // CPU에서는 Pose만 넘김 (offset은 따로) 
    for (int i = 0; i < (int)m_Skeleton.size(); ++i)
    {
        Matrix pose = m_Skeleton[i].m_Model;
        m_SkeletonPose.SetMatrix(i, pose.Transpose());
    }

    //for (auto& bone : m_Skeleton)
    //{
    //    XMFLOAT4X4 localMat, modelMat;
    //    XMStoreFloat4x4(&localMat, bone.m_Local);
    //    XMStoreFloat4x4(&modelMat, bone.m_Model);

    //    char buf[512];
    //    sprintf_s(buf, "%s: LocalPos=(%.3f, %.3f, %.3f), ModelPos=(%.3f, %.3f, %.3f)\n",
    //        bone.m_Name.c_str(),
    //        localMat._41, localMat._42, localMat._43, modelMat._41, modelMat._42, modelMat._43);
    //    OutputDebugStringA(buf);
    //}

    for (int i = 0; i < 10; ++i)
    {
        Matrix model = m_Skeleton[i].m_Model;
        Matrix offset = m_pSkeletonInfo->BoneOffsetMatrices.GetMatrix(i);

        Matrix finalMatA = offset * model; // HLSL 쪽이 gBoneOffset * gBonePose 이므로 이쪽이 실제 사용
        Matrix finalMatB = model * offset; // 순서 비교용

        char buf[512];
        sprintf_s(buf,
            "[BONE %02d] %-25s | ModelPos(%.2f, %.2f, %.2f) | OffsetPos(%.2f, %.2f, %.2f) | FinalA._41=%.2f, FinalB._41=%.2f\n",
            i, m_Skeleton[i].m_Name.c_str(),
            model._41, model._42, model._43,
            offset._41, offset._42, offset._43,
            finalMatA._41, finalMatB._41);
        OutputDebugStringA(buf);
    }
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