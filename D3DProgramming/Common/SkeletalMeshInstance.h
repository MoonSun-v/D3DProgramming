#pragma once
#include <memory>
#include <d3d11.h>
#include "SkeletalMeshAsset.h"
#include "Transform.h"
#include "Animator.h"
#include "AnimationController.h"

class SkeletalMeshInstance
{
public:
    std::string m_Name;
	std::shared_ptr<SkeletalMeshAsset> m_Asset; // 공유되는 SkeletalMeshAsset
    
    Transform transform;

    AnimationController m_Controller;

    BoneMatrixContainer m_SkeletonPose;

    ComPtr<ID3D11Buffer> m_pBonePoseBuffer; // b1

public:
    // void Initialize(const SkeletonInfo* skeleton);
    void SetAsset(ID3D11Device* device, std::shared_ptr<SkeletalMeshAsset> asset);
    void Update(float deltaTime);
    void Render(ID3D11DeviceContext* context, ID3D11SamplerState* sampler, int isRigid);
	void RenderShadow(ID3D11DeviceContext* context, int isRigid);

    Matrix GetWorld() const { return transform.GetMatrix(); }
};