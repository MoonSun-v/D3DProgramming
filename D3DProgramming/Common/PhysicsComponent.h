#pragma once
#include <PxPhysicsAPI.h>
#include <memory>
#include <directxtk/SimpleMath.h>
#include <unordered_set>

#include "CollisionLayer.h"
#include "MeshInstanceBase.h"

using namespace DirectX::SimpleMath;
using namespace physx;

class Transform;
class SkeletalMeshInstance;

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
    Quaternion localRotation = Quaternion::Identity; // 회전 아직 적용X 사용하려나..? 

    bool isTrigger = false;
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
    MeshInstanceBase* owner = nullptr; // 소유 객체
    Transform* transform = nullptr;

    // Collision / Trigger 이벤트 관리 
    std::unordered_set<PhysicsComponent*> m_CollisionActors;  // 현재 접촉 Actor
    std::unordered_set<PhysicsComponent*> m_TriggerActors;    // 현재 Trigger Actor
    
    // CCT 전용 충돌 상태 (프레임 비교)
    std::unordered_set<PhysicsComponent*> m_CCTPrevContacts;
    std::unordered_set<PhysicsComponent*> m_CCTCurrContacts;
    // CCT 전용 Trigger 상태
    std::unordered_set<PhysicsComponent*> m_CCTPrevTriggers;
    std::unordered_set<PhysicsComponent*> m_CCTCurrTriggers;

    // Trigger 수집용
    std::unordered_set<PhysicsComponent*> m_PendingTriggers;

public:
    bool IsTrigger() const { return m_IsTrigger; }

private:
    bool m_IsTrigger = false;

private:
    PxRigidActor* m_Actor = nullptr;
    PxShape* m_Shape = nullptr;
	
    PhysicsBodyType m_BodyType;
    ColliderType m_ColliderType;

    CollisionLayer m_Layer = CollisionLayer::Default;
    CollisionMask  m_Mask = 0xFFFFFFFF;

public:
    // -------------- (임시) CCT --------------
    PxController* m_Controller = nullptr;       // (임시) 캐릭터 컨트롤러 
    Vector3 m_ControllerOffset = { 0, 0, 0 };   // CCT 전용 오프셋
    const float m_MinDown = -1.0f;              // 바닥 접촉 유지용 미세 하강
    const float m_MoveSpeed = 2.0f;
    PxFilterData m_CCTFilterData;               // CCT 전용 FilterData

    // PhysicsComponent.h
    float m_VerticalVelocity = 0.0f;
    bool  m_RequestJump = false;
    float m_JumpSpeed = 5.5f; // m/s (체감 좋은 값이라고 함)

public:
    PhysicsComponent() = default;
    ~PhysicsComponent();

    // -----------------------------
    // Collision / Trigger 이벤트 콜백 
    // -----------------------------
    virtual void OnCollisionEnter(PhysicsComponent* other) { OutputDebugStringW(L"[PhysicsComponent] Collision Enter! \n"); }
    virtual void OnCollisionStay(PhysicsComponent* other) { /*OutputDebugStringW(L"[PhysicsComponent] Collision Stay! \n");*/ }
    virtual void OnCollisionExit(PhysicsComponent* other) { OutputDebugStringW(L"[PhysicsComponent] Collision Exit! \n"); }

    virtual void OnTriggerEnter(PhysicsComponent* other) { OutputDebugStringW(L"[PhysicsComponent] Trigger Enter! \n"); }
    virtual void OnTriggerStay(PhysicsComponent* other) { /*OutputDebugStringW(L"[PhysicsComponent] Trigger Stay \n");*/ }
    virtual void OnTriggerExit(PhysicsComponent* other) { OutputDebugStringW(L"[PhysicsComponent] Trigger Exit! \n"); }


    // --------------------------
    // 외부 API : Collider / Actor 생성 
    // --------------------------
    void CreateStaticBox(const Vector3& half, const Vector3& localOffset = { 0,0,0 });
    void CreateTriggerBox(const Vector3& half, const Vector3& localOffset = { 0,0,0 }); // Trigger는 Static Actor가 정석
    void CreateDynamicBox(const Vector3& half, float density = 1.0f, const Vector3& localOffset = { 0,0,0 });

    void CreateStaticSphere(float radius, const Vector3& localOffset = { 0,0,0 });
    void CreateDynamicSphere(float radius, float density = 1.0f, const Vector3& localOffset = { 0,0,0 });

    void CreateStaticCapsule(float radius, float height, const Vector3& localOffset = {0,0,0});
    void CreateTriggerCapsule(float radius, float height, const Vector3& localOffset = { 0,0,0 });
    void CreateDynamicCapsule(float radius, float height, float density = 1.0f, const Vector3& localOffset = {0,0,0});

    // -------------- (임시) CCT --------------
    void CreateCharacterCapsule(float radius, float height, const Vector3& localOffset); // Character Controller -> 캡슐 컨트롤러 생성
    void MoveCharacter(const Vector3& wishDir, float fixedDt);   // 이동 
    void Jump();

    void ResolveCCTCollisions();
    void ResolveCCTTriggers();
    void CheckCCTTriggers();

    // --------------------------
    // Transform 연동
    // --------------------------
    void SyncToPhysics();
    void SyncFromPhysics();

    // --------------------------
    // 충돌 레이어 
    // --------------------------
    void SetLayer(CollisionLayer layer);
    CollisionLayer GetLayer() const { return m_Layer; }

private:
    void ApplyFilter();

private:
    // --------------------------
    // 내부 생성기
    // --------------------------
    void CreateCollider(ColliderType collider, PhysicsBodyType body, const ColliderDesc& desc);
};