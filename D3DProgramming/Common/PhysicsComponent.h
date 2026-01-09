#pragma once
#include <PxPhysicsAPI.h>
#include <memory>
#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;
using namespace physx;

class Transform;

// ------------------------------
// Enum (엔진 내부용)
// ------------------------------
enum class PhysicsBodyType
{
    Static,
    Dynamic,
    Kinematic
};

enum class ColliderType
{
    Box,
    Sphere,
    Capsule
};

// ------------------------------
// Collider Desc
// ------------------------------
struct ColliderDesc
{
    Vector3 halfExtents = { 1,1,1 }; // Box
    float radius = 0.5f;           // Sphere / Capsule
    float height = 1.0f;           // Capsule
    float density = 1.0f;          // Dynamic
};


// ----------------------------------------------------
// [ [엔진 Transform]  ←→  [PhysX RigidActor] ]
// - 엔진 쪽 Transform (position / rotation) 을 가지고 있는 GameObject(owner) 와 
// - PhysX 쪽 PxRigidActor(m_Actor)
//   이 둘을 양방향으로 동기화 하는 역할 
// ----------------------------------------------------
class PhysicsComponent
{
public:
    Transform* owner = nullptr;

private:
    PxRigidActor* m_Actor = nullptr;
    PxShape* m_Shape = nullptr;

    PhysicsBodyType m_BodyType;
    ColliderType m_ColliderType;

public:
    ~PhysicsComponent();

    // --------------------------
    // 외부 API 
    // --------------------------
    void CreateStaticBox(const Vector3& half);
    void CreateDynamicBox(const Vector3& half, float density = 1.0f);

    void CreateStaticSphere(float radius);
    void CreateDynamicSphere(float radius, float density = 1.0f);

    void CreateStaticCapsule(float radius, float height);
    void CreateDynamicCapsule(float radius, float height, float density = 1.0f);

    // --------------------------
    // Sync
    // --------------------------
    void SyncToPhysics();
    void SyncFromPhysics();


private:
    // --------------------------
    // 내부 생성기
    // --------------------------
    void CreateCollider(
        ColliderType collider,
        PhysicsBodyType body,
        const ColliderDesc& desc
    );
};
