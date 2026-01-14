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

    Vector3 localOffset = { 0, 0, 0 };    
    Quaternion localRotation = Quaternion::Identity; // 회전 아직 적용X 
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
    Transform* transform = nullptr;

private:
    PxRigidActor* m_Actor = nullptr;
	PxController* m_Controller = nullptr; // 임시 캐릭터 컨트롤러 용도 (추후 분리 예정)

    PxShape* m_Shape = nullptr;

    PhysicsBodyType m_BodyType;
    ColliderType m_ColliderType;

    Vector3 m_ControllerOffset = { 0, 0, 0 };   // CCT 전용 오프셋 (추후 분리 예정)
    const float m_MinDown = -1.0f;              // 바닥 접촉 유지용 미세 하강
    const float m_MoveSpeed = 200.0f;

public:
    ~PhysicsComponent();

    // --------------------------
    // 외부 API 
    // --------------------------
    void CreateStaticBox(const Vector3& half, const Vector3& localOffset = { 0,0,0 });
    void CreateDynamicBox(const Vector3& half, float density = 1.0f, const Vector3& localOffset = { 0,0,0 });

    void CreateStaticSphere(float radius, const Vector3& localOffset = { 0,0,0 });
    void CreateDynamicSphere(float radius, float density = 1.0f, const Vector3& localOffset = { 0,0,0 });

    void CreateStaticCapsule(float radius, float height, const Vector3& localOffset = {0,0,0});
    void CreateDynamicCapsule(float radius, float height, float density = 1.0f, const Vector3& localOffset = {0,0,0});

    // -------------- CCT 임시 --------------
    void CreateCharacterCapsule(float radius, float height, const Vector3& localOffset); // Character Controller -> 캡슐 컨트롤러 생성
    void MoveCharacter(const Vector3& wishDir, float fixedDt);   // 이동 

    // --------------------------
    // Sync
    // --------------------------
    void SyncToPhysics();
    void SyncFromPhysics();


private:
    // --------------------------
    // 내부 생성기
    // --------------------------
    void CreateCollider(ColliderType collider, PhysicsBodyType body, const ColliderDesc& desc);
};
