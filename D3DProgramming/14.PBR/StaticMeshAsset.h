#pragma once
#include <vector>
#include <string>
#include <wrl/client.h>
#include <d3d11.h>
#include "Material.h"
#include "SkeletalMeshSection.h"

using Microsoft::WRL::ComPtr;

class StaticMeshAsset
{
public:
    std::vector<SkeletalMeshSection> m_Sections;
    std::vector<Material> m_Materials;

    ComPtr<ID3D11Buffer> m_pVertexBuffer;
    ComPtr<ID3D11Buffer> m_pIndexBuffer;

public:
    bool LoadFromFBX(ID3D11Device* device, const std::string& path);
    void Clear();
};