#pragma once
#include <memory>
#include "StaticMeshAsset.h"
#include "Transform.h"
#include "PhysicsComponent.h"
#include "MeshInstanceBase.h"

class StaticMeshInstance : public MeshInstanceBase
{
public:
    std::string m_Name;
    std::shared_ptr<StaticMeshAsset> m_Asset;
    std::unique_ptr<PhysicsComponent> physics;
    Transform transform;

public:
    void SetAsset(std::shared_ptr<StaticMeshAsset> asset);
    void Render(ID3D11DeviceContext* context, ID3D11SamplerState* pSampler);
    void RenderShadow(ID3D11DeviceContext* context);
    void Update();

    Matrix GetWorld() const { return transform.GetMatrix(); }
    virtual std::string GetName() const override { return m_Name; }
};
