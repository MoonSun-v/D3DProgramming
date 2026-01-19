#pragma once
#include <directxtk/SimpleMath.h>
#include <string>

using namespace DirectX::SimpleMath;

class MeshInstanceBase
{
public:
    virtual ~MeshInstanceBase() = default;

    virtual std::string GetName() const = 0;

    // юс╫ц 
    // virtual Vector3 GetPosition() const = 0;
    // virtual void SetPosition(const Vector3& pos) = 0;
};
