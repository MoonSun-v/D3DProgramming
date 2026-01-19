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

    void SetRotationY(float yaw)
    {
        rotation = Quaternion::CreateFromYawPitchRoll(yaw, 0.0f, 0.0f);
    }

    float GetYaw() const
    {
        using namespace DirectX;

        XMVECTOR q = XMLoadFloat4(&rotation);

        // 회전을 행렬로 변환
        XMMATRIX m = XMMatrixRotationQuaternion(q);

        // Forward 벡터 추출 (Z+ Forward)
        XMVECTOR forward = m.r[2]; // Z축 Forward
        forward = XMVector3Normalize(forward);

        // Yaw 계산
        float yaw = atan2f(XMVectorGetX(forward), XMVectorGetZ(forward));

        return yaw;
    }

    XMFLOAT3 GetForward() const
    {
        using namespace DirectX;
        XMVECTOR q = XMLoadFloat4(&rotation);
        XMMATRIX rotMat = XMMatrixRotationQuaternion(q);

        // Z+ 방향을 Forward로 사용
        XMVECTOR forward = rotMat.r[2];

        // 반대 방향 (Z-) 으로 반전 
        forward = XMVectorNegate(forward);

        forward = XMVector3Normalize(forward);

        XMFLOAT3 f;
        XMStoreFloat3(&f, forward);
        return f;
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