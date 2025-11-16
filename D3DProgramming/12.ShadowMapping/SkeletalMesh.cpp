#include "SkeletalMesh.h"
#include "Bone.h"
#include "../Common/D3DDevice.h"
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
    // [ 본 행렬용 상수 버퍼 : b1 (Pose) ]
    D3D11_BUFFER_DESC bdBone = {};
    bdBone.Usage = D3D11_USAGE_DEFAULT;
    bdBone.ByteWidth = sizeof(BoneMatrixContainer);
    bdBone.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bdBone.CPUAccessFlags = 0;
    HR_T(device->CreateBuffer(&bdBone, nullptr, m_pBonePoseBuffer.GetAddressOf()));

    // [ 본 오프셋용 상수 버퍼 : b2 (Offset) ]
    HR_T(device->CreateBuffer(&bdBone, nullptr, m_pBoneOffsetBuffer.GetAddressOf()));


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

    if (hasAnimation) // 애니메이션 있으면 Animation 리스트 채우기
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
        // Skinned이면 skeleton 공유, static이면 nullptr
        // TODO : static이면 submesh null 처리 해야함 
        m_Sections[i].m_pSkeletonInfo = m_pSkeletonInfo.get(); 
        m_Sections[i].InitializeFromAssimpMesh(device, scene->mMeshes[i]);
        m_Sections[i].m_MaterialIndex = scene->mMeshes[i]->mMaterialIndex;
    }
    
    return true;
}

void SkeletalMesh::CreateSkeleton(const aiScene* scene)
{
    if (!scene->mRootNode) return;

    bool hasBones = false;
    for (UINT meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
        if (scene->mMeshes[meshIdx]->HasBones()) { hasBones = true; break; }

    if (!hasBones) return; // StaticMesh인 경우 건너뜀

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

                bone.m_Model = bone.m_Local; // 바인드 포즈에서는 누적 X 

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


void SkeletalMesh::Render(ID3D11DeviceContext* context, ID3D11SamplerState* pSampler, int isRigid)
{
    if(isRigid == 0)
    {
        // Bone Pose (b1)
        context->UpdateSubresource(m_pBonePoseBuffer.Get(), 0, nullptr, m_SkeletonPose.m_Model, 0, 0);
        context->VSSetConstantBuffers(1, 1, m_pBonePoseBuffer.GetAddressOf());

        // Bone Offset (b2) 
        if (m_pSkeletonInfo)
        {
            context->UpdateSubresource(m_pBoneOffsetBuffer.Get(), 0, nullptr, m_pSkeletonInfo->BoneOffsetMatrices.m_Model, 0, 0);
            context->VSSetConstantBuffers(2, 1, m_pBoneOffsetBuffer.GetAddressOf());
        }
	}

    for (auto& sub : m_Sections)
    {
        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Materials.size())
        {
            material = &m_Materials[sub.m_MaterialIndex];
        }

        if (material)
        {
            sub.Render(context, *material, pSampler); 
        }
    }
}

void SkeletalMesh::RenderShadow(ID3D11DeviceContext* context, int isRigid)
{
    if (isRigid == 0)
    {
        // Bone Pose (b1)
        context->UpdateSubresource(m_pBonePoseBuffer.Get(), 0, nullptr, m_SkeletonPose.m_Model, 0, 0);
        context->VSSetConstantBuffers(1, 1, m_pBonePoseBuffer.GetAddressOf());

        // Bone Offset (b2)
        if (m_pSkeletonInfo)
        {
            context->UpdateSubresource(m_pBoneOffsetBuffer.Get(), 0, nullptr, m_pSkeletonInfo->BoneOffsetMatrices.m_Model, 0, 0);
            context->VSSetConstantBuffers(2, 1, m_pBoneOffsetBuffer.GetAddressOf());
        }
    }

    for (auto& sub : m_Sections)
    {
        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, sub.m_VertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(sub.m_IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

        context->DrawIndexed(sub.m_IndexCount, 0, 0);
    }
}


void SkeletalMesh::Update(float deltaTime, const Matrix& worldTransform)
{
    if (m_Skeleton.empty()) // 본이 없으면(StaticMesh) 업데이트할 게 없음
    {
        m_World = worldTransform;
        return; 
    }

    if (m_Animations.empty()) return;

    Animation& anim = m_Animations[m_AnimationIndex];
    m_AnimationTime += deltaTime;
    if (m_AnimationTime > anim.Duration)
    {
        m_AnimationTime = fmod(m_AnimationTime, anim.Duration);
    }

    XMMATRIX worldMat = XMLoadFloat4x4(&worldTransform); // SimpleMath::Matrix -> XMMATRIX

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

        if (bone.m_ParentIndex != -1)
        {
            bone.m_Model = bone.m_Local * m_Skeleton[bone.m_ParentIndex].m_Model;
        }
        else 
        {
            bone.m_Model = bone.m_Local * worldMat; // 조정 : 루트 본만 적용  
        }
    }

    // GPU용 본 행렬 계산
    m_SkeletonPose.SetBoneCount((int)m_Skeleton.size());

    // CPU에서는 Pose만 넘김 (offset은 GPU에서 연산) 
    for (int i = 0; i < (int)m_Skeleton.size(); ++i)
    {
        Matrix pose = m_Skeleton[i].m_Model;
        m_SkeletonPose.SetMatrix(i, pose.Transpose());
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