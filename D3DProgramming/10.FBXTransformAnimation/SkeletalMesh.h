#pragma once
#include <vector>
#include <string>

#include "SkeletalMeshSection.h"
#include "Material.h"
#include "Animation.h"
#include "SkeletonInfo.h"
#include "Bone.h"


class SkeletalMesh
{
public:
    std::vector<SkeletalMeshSection> m_Sections;     // SubMesh 단위 데이터
    std::vector<Material> m_Materials;               // FBX에서 읽은 머티리얼들
    std::vector<Animation> m_Animations;             // 애니메이션 리스트
    std::vector<Bone> m_Skeleton;                    // Bone 인스턴스 데이터

    float m_AnimationTime = 0.0f;            // 현재 애니메이션 시간
    int m_AnimationIndex = 0;                        // 현재 재생 중인 애니메이션 인덱스

    XMMATRIX m_World;                                // 월드 행렬
   
    // BoneMatrixContainer m_SkeletonPose;              // GPU로 전송할 본 행렬 컨테이너
    // SkeletonInfo m_SkeletonInfo;


public:
    SkeletalMesh();

    // [ FBX 파일로부터 SkeletalMesh 로드 ]
    bool LoadFromFBX(ID3D11Device* device, const std::string& path);
    void Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11Buffer* pBoneBuffer, ID3D11SamplerState* pSampler);
    void Clear();
    void Update(float deltaTime);
    // void UpdateBoneBuffer(ID3D11DeviceContext* context, ID3D11Buffer* pBoneBuffer);

private:
   // void ReadSkeletalMeshFile(const std::string& path);
   // void ReadAnimationFile(const std::string& path);
    void CreateSkeleton(const aiScene* scene);
};