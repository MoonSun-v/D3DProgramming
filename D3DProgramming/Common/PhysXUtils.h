#pragma once
#include <PxPhysicsAPI.h>
#include <DirectXMath.h>

using namespace physx;
using namespace DirectX;

// ------------------------------
// 좌표계 변환 (LH ↔ RH)
// ------------------------------

// [ Position ]
inline PxVec3 ToPx(const XMFLOAT3 & v)
{
    return PxVec3(v.x, v.y, v.z);
}

inline XMFLOAT3 ToDX(const PxVec3 & v)
{
    return { v.x, v.y, v.z };
}

// [ Quaternion ]
inline PxQuat ToPxQuat(const XMVECTOR & q)
{
    XMFLOAT4 f;
    XMStoreFloat4(&f, q);
    return PxQuat(f.x, f.y, f.z, f.w);
}

inline XMVECTOR ToDXQuat(const PxQuat & q)
{
    return XMVectorSet(q.x, q.y, q.z, q.w);
}

// [ PxQuat → XMFLOAT4 변환 ]
inline XMFLOAT4 ToDXQuatF4(const PxQuat& q)
{
    return XMFLOAT4(q.x, q.y, q.z, q.w);
}
