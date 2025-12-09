#pragma once
#include <memory>
#include "StaticMeshAsset.h"
#include "Transform.h"

class StaticMeshInstance
{
public:
    std::shared_ptr<StaticMeshAsset> m_Asset;
    Transform transform;

public:
    void SetAsset(std::shared_ptr<StaticMeshAsset> asset);
    void Render(ID3D11DeviceContext* context, ID3D11SamplerState* pSampler);
    void RenderShadow(ID3D11DeviceContext* context);

    Matrix GetWorld() const { return transform.GetMatrix(); }
};
