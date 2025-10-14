#pragma once
#include <vector>
#include <string>

#include "StaticMeshSection.h"
#include "Material.h"

class StaticMesh
{
private:
    std::vector<StaticMeshSection> m_SubMeshes;
    std::vector<Material> m_Materials;

    DirectX::XMMATRIX m_World;

public:
    StaticMesh();

    // [ FBX 파일로부터 StaticMesh 로드 ]
    bool LoadFromFBX(ID3D11Device* device, const std::string& path);

    void Render(ID3D11DeviceContext* context);
};