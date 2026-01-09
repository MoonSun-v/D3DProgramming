#include "SkeletalMeshInstance.h"
#include "../Common/D3DDevice.h"

void SkeletalMeshInstance::SetAsset(ID3D11Device* device, std::shared_ptr<SkeletalMeshAsset> asset)
{
    m_Asset = asset;

    // [ 본 행렬용 상수 버퍼 b1 ]
    D3D11_BUFFER_DESC bdBone = {};
    bdBone.Usage = D3D11_USAGE_DEFAULT;
    bdBone.ByteWidth = sizeof(BoneMatrixContainer);
    bdBone.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bdBone.CPUAccessFlags = 0;
    HR_T(device->CreateBuffer(&bdBone, nullptr, m_pBonePoseBuffer.GetAddressOf()));

    m_Controller.Initialize(m_Asset->m_pSkeletonInfo.get());
}

void SkeletalMeshInstance::Update(float deltaTime)
{
    if (!m_Asset || m_Asset->m_Skeleton.empty()) return;
    if (m_Asset->m_Animations.empty()) return;

    if (physics)
        physics->SyncFromPhysics();

    // -------------------------------
    // 업데이트 
    // -------------------------------
    m_Controller.Update(deltaTime);


    // -------------------------------
    // 본 행렬 계산
    // -------------------------------
    const auto& pose = m_Controller.AnimatorInstance.GetFinalPose();
    XMMATRIX worldMat = transform.GetMatrix();

    for (int i = 0; i < (int)m_Asset->m_Skeleton.size(); ++i)
    {
        auto& bone = m_Asset->m_Skeleton[i];

        bone.m_Local = pose[i];
        
        if (bone.m_ParentIndex != -1)
        {
            bone.m_Model = bone.m_Local * m_Asset->m_Skeleton[bone.m_ParentIndex].m_Model;
        }
        else
        {
            bone.m_Model = bone.m_Local * worldMat;
        }
    }

    // -------------------------------
    // GPU용 본 행렬 (Final Matrix)
    // -------------------------------
    m_SkeletonPose.SetBoneCount((int)m_Asset->m_Skeleton.size());
    for (int i = 0; i < (int)m_Asset->m_Skeleton.size(); ++i)
    {
        Matrix finalMat = m_Asset->m_Skeleton[i].m_Model; 
        m_SkeletonPose.SetMatrix(i, finalMat.Transpose());
    }
}


void SkeletalMeshInstance::Render(ID3D11DeviceContext* context, ID3D11SamplerState* pSampler, int isRigid)
{
    if (!m_Asset) return;

    if (isRigid == 0)
    {
        // Bone Pose (b1)
        context->UpdateSubresource(m_pBonePoseBuffer.Get(), 0, nullptr, m_SkeletonPose.m_Model, 0, 0);
        context->VSSetConstantBuffers(1, 1, m_pBonePoseBuffer.GetAddressOf());

        // Bone Offset (b2) 
        if (m_Asset->m_pSkeletonInfo)
        {
            context->UpdateSubresource(m_Asset->m_pBoneOffsetBuffer.Get(), 0, nullptr, m_Asset->m_pSkeletonInfo->BoneOffsetMatrices.m_Model, 0, 0);
            context->VSSetConstantBuffers(2, 1, m_Asset->m_pBoneOffsetBuffer.GetAddressOf());
        }
    }

    for (auto& sub : m_Asset->m_Sections)
    {
        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Asset->m_Materials.size())
        {
            material = &m_Asset->m_Materials[sub.m_MaterialIndex];
        }

        if (material)
        {
            sub.Render(context, *material, pSampler);
        }
    }
}

void SkeletalMeshInstance::RenderShadow(ID3D11DeviceContext* context, int isRigid)
{
    if (!m_Asset) return;

    if (isRigid == 0)
    {
        // // Bone Pose (b1)
        context->UpdateSubresource(m_pBonePoseBuffer.Get(), 0, nullptr, m_SkeletonPose.m_Model, 0, 0);
        context->VSSetConstantBuffers(1, 1, m_pBonePoseBuffer.GetAddressOf());

        // Bone Offset (b2)
        if (m_Asset->m_pSkeletonInfo)
        {
            context->UpdateSubresource(m_Asset->m_pBoneOffsetBuffer.Get(), 0, nullptr, m_Asset->m_pSkeletonInfo->BoneOffsetMatrices.m_Model, 0, 0);
            context->VSSetConstantBuffers(2, 1, m_Asset->m_pBoneOffsetBuffer.GetAddressOf());
        }
    }

    for (auto& sub : m_Asset->m_Sections)
    {
        // Shadow Pass: Opacity SRV만 slot 5에 바인딩
        Material* material = nullptr;
        if (sub.m_MaterialIndex >= 0 && sub.m_MaterialIndex < (int)m_Asset->m_Materials.size())
            material = &m_Asset->m_Materials[sub.m_MaterialIndex];

        ID3D11ShaderResourceView* opacitySRV[1] =
        {
            material ? material->GetTextures().OpacitySRV.Get() : nullptr
        };

        context->PSSetShaderResources(5, 1, opacitySRV);

        UINT stride = sizeof(Vertex);
        UINT offset = 0;
        context->IASetVertexBuffers(0, 1, sub.m_VertexBuffer.GetAddressOf(), &stride, &offset);
        context->IASetIndexBuffer(sub.m_IndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
        context->DrawIndexed(sub.m_IndexCount, 0, 0);
    }
}
