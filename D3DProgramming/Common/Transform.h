#pragma once
#include <directxtk/SimpleMath.h>

class Transform
{
public:
	XMFLOAT3 position = { 0,0,0 };
	XMFLOAT4 rotation = { 0,0,0,1 }; // Quaternion (내부 기준)
	XMFLOAT3 scale = { 1,1,1 };

public:
    // ----------------------------
    // 입력용 API (Degree Euler)
    // Degree로 입력 받고 -> Quaternion으로 변환 해줌 
    // ----------------------------
    void SetRotationDegree(const DirectX::XMFLOAT3& eulerDeg)
    {
        using namespace DirectX;

        XMFLOAT3 rad =
        {
            XMConvertToRadians(eulerDeg.x),
            XMConvertToRadians(eulerDeg.y),
            XMConvertToRadians(eulerDeg.z)
        };

        XMVECTOR q = XMQuaternionRotationRollPitchYaw(
            rad.x, rad.y, rad.z);

        XMStoreFloat4(&rotation, q);
    }

    // ----------------------------
    // 내부 계산용
    // ----------------------------
	XMMATRIX GetMatrix() const
	{
		XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);
		XMMATRIX R = XMMatrixRotationQuaternion(XMLoadFloat4(&rotation));
		XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);
		return S * R * T;
	}
};