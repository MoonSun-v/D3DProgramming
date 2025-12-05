#include "StaticMeshInstance.h"

void StaticMeshInstance::SetAsset(std::shared_ptr<StaticMeshAsset> asset)
{
    m_Asset = asset;
}

void StaticMeshInstance::Render(ID3D11DeviceContext* context, ID3D11SamplerState* pSampler)
{
    if (!m_Asset) return;

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

void StaticMeshInstance::RenderShadow(ID3D11DeviceContext* context)
{
    if (!m_Asset) return;

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

