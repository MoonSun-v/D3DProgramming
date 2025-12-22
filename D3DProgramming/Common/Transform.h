#pragma once
#include <directxtk/SimpleMath.h>

class Transform
{
public:
	XMFLOAT3 position = { 0,0,0 };
	XMFLOAT3 rotation = { 0,0,0 }; // Euler (radians)
	XMFLOAT3 scale = { 1,1,1 };

public:
	XMMATRIX GetMatrix() const
	{
		XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX R = XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
		XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
		return S * R * T;
	}
};