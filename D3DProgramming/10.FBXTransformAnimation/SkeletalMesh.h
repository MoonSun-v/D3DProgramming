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
    std::vector<SkeletalMeshSection> m_Sections;     // SubMesh ���� ������
    std::vector<Material> m_Materials;               // FBX���� ���� ��Ƽ�����
    std::vector<Animation> m_Animations;             // �ִϸ��̼� ����Ʈ

    float m_AnimationProgressTime = 0.0f;            // ���� �ִϸ��̼� �ð�
    int m_AnimationIndex = 0;                        // ���� ��� ���� �ִϸ��̼� �ε���

    std::vector<Bone> m_Skeleton;                    // Bone �ν��Ͻ� ������
    BoneMatrixContainer m_SkeletonPose;              // GPU�� ������ �� ��� �����̳�

    XMMATRIX m_World;                                // ���� ���


public:
    SkeletalMesh();

    // [ FBX ���Ϸκ��� SkeletalMesh �ε� ]
    bool LoadFromFBX(ID3D11Device* device, const std::string& path);
    void Render(ID3D11DeviceContext* context, const ConstantBuffer& globalCB, ID3D11Buffer* pCB, ID3D11Buffer* pBoneBuffer, ID3D11SamplerState* pSampler);
    void Clear();
    void Update(float deltaTime);
    void UpdateBoneBuffer(ID3D11DeviceContext* context, ID3D11Buffer* pBoneBuffer);

private:
    void ReadSkeletalMeshFile(const std::string& path);
    void ReadAnimationFile(const std::string& path);
    void CreateSkeleton(const aiScene* scene);
};