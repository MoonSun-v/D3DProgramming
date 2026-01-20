#pragma once
#include <PxPhysicsAPI.h>
#include <DirectXMath.h>

using namespace physx;
using namespace DirectX;


// ------------------------------------------
// Render / Transform   : 1 unit = 1 cm
// PhysX                : 1 unit = 1 m
constexpr float WORLD_TO_PHYSX = 0.01f; // cm -> m
constexpr float PHYSX_TO_WORLD = 100.0f;
// ------------------------------------------



// ------------------------------
// 좌표계 변환 (LH ↔ RH)
// ------------------------------
// 
// 위치 -> ToDX(PxVec3)
// 크기 -> PxToDX(float)
// 회전 -> ToDXQuat / ToDXQuatF4



// [ Position ]
inline PxVec3 ToPx(const XMFLOAT3& v)
{
    return PxVec3(
        v.x * WORLD_TO_PHYSX,
        v.y * WORLD_TO_PHYSX,
        v.z * WORLD_TO_PHYSX
    );
}
inline XMFLOAT3 ToDX(const PxVec3& v)
{
    return {
        v.x * PHYSX_TO_WORLD,
        v.y * PHYSX_TO_WORLD,
        v.z * PHYSX_TO_WORLD
    };
}

// PxVec3 -> XMVECTOR 변환
inline XMVECTOR ToDXVec3(const PxVec3& v)
{
    return XMVectorSet(
        v.x * PHYSX_TO_WORLD,
        v.y * PHYSX_TO_WORLD,
        v.z * PHYSX_TO_WORLD,
        0.0f // w = 0
    );
}
//XMFLOAT3 p = ToDX(worldPose.p);
//XMVECTOR pos = XMLoadFloat3(&p);


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


// [ Scale ]
inline float PxToDX(float v)
{
    return v * PHYSX_TO_WORLD;
}
