#pragma once
#include <vector>
#include <string>
#include "MeshSection.h"
#include "Material.h"
#include "AnimationClip.h"
#include "Bone.h"

// 기존 SkeletalMesh에서 Asset부분(공유되는 부분)만 추출 

class SkeletalMeshAsset
{
public:
    std::vector<MeshSection> m_Sections;
    std::vector<Material> m_Materials;
    std::vector<Bone> m_Skeleton;
    std::unique_ptr<SkeletonInfo> m_pSkeletonInfo;

    std::vector<AnimationClip> m_Animations;

	ComPtr<ID3D11Buffer> m_pBoneOffsetBuffer; // b2 : Bone Offset

public:

    // [ 스켈레탈 ]
    bool LoadFromFBX(ID3D11Device* device, const std::string& path);
    void CreateSkeleton(const aiScene* scene);
    const SkeletonInfo* GetSkeletonInfo() const
    {
        return m_pSkeletonInfo.get();
    }


    // [ 애니메이션 ] 
    bool LoadAnimationFromFBX(const std::string& path, const std::string& overrideName);
    void AddAnimation(AnimationClip&& anim)
    {
        m_Animations.push_back(std::move(anim));
    }

	int FindAnimationIndexByName(const std::string& name) const;
	const AnimationClip* GetAnimation(int index) const;
    const AnimationClip* GetAnimation(const std::string& name) const;

    void Clear();
};