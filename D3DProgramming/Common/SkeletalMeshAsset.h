#pragma once
#include <vector>
#include <string>
#include "MeshSection.h"
#include "Material.h"
#include "Animation.h"
#include "Bone.h"

// 기존 SkeletalMesh에서 Asset부분(공유되는 부분)만 추출 

class SkeletalMeshAsset
{
public:
    std::vector<MeshSection> m_Sections;
    std::vector<Material> m_Materials;
    std::vector<Animation> m_Animations;
    std::vector<Bone> m_Skeleton;
    std::unique_ptr<SkeletonInfo> m_pSkeletonInfo;

	ComPtr<ID3D11Buffer> m_pBoneOffsetBuffer; // b2 : Bone Offset

public:
    bool LoadFromFBX(ID3D11Device* device, const std::string& path);
    bool LoadAnimationFromFBX(const std::string& path, const std::string& overrideName);
	int FindAnimationIndexByName(const std::string& name) const;
	const Animation* GetAnimation(int index) const;
    void Clear();

    void AddAnimation(Animation&& anim)
    {
        m_Animations.push_back(std::move(anim));
    }

    const SkeletonInfo* GetSkeletonInfo() const
    {
        return m_pSkeletonInfo.get();
    }

private:
    void CreateSkeleton(const aiScene* scene);
};