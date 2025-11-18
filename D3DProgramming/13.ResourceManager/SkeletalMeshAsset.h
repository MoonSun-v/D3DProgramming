#pragma once
#include <vector>
#include <string>
#include "SkeletalMeshSection.h"
#include "Material.h"
#include "Animation.h"
#include "Bone.h"

// 기존 SkeletalMesh에서 Asset부분(공유되는 부분)만 추출 

// m_pBonePoseBuffer 제거해야 함.
// 포즈는 인스턴스별로 다름 -> Asset에 들어가면 모든 객체가 똑같이 움직여버림.

class SkeletalMeshAsset
{
public:
    std::vector<SkeletalMeshSection> m_Sections;
    std::vector<Material> m_Materials;
    std::vector<Animation> m_Animations;
    std::vector<Bone> m_Skeleton;
    std::unique_ptr<SkeletonInfo> m_pSkeletonInfo;

	ComPtr<ID3D11Buffer> m_pBoneOffsetBuffer; // b2 : Bone Offset

public:
    bool LoadFromFBX(ID3D11Device* device, const std::string& path);

private:
    void CreateSkeleton(const aiScene* scene);
};