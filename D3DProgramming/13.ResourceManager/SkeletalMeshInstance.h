#pragma once
#include <memory>
#include <d3d11.h>

#include "SkeletalMeshAsset.h"

class SkeletalMeshInstance
{
public:
	std::shared_ptr<SkeletalMeshAsset> m_Asset; // 공유되는 SkeletalMeshAsset

    Matrix m_World; // 월드 행렬 

	float m_AnimationTime = 0.0f;   // 현재 애니메이션 시간
	int m_AnimationIndex = 0;       // 현재 재생 중인 애니메이션 인덱스

    BoneMatrixContainer m_SkeletonPose;

    ComPtr<ID3D11Buffer> m_pBonePoseBuffer; // b1

public:
    void SetAsset(ID3D11Device* device, std::shared_ptr<SkeletalMeshAsset> asset);
    void Update(float deltaTime, const Matrix& worldTransform);
    void Render(ID3D11DeviceContext* context, ID3D11SamplerState* sampler, int isRigid);
	void RenderShadow(ID3D11DeviceContext* context, int isRigid);
};