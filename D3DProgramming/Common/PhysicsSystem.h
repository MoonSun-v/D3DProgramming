#pragma once
#include <PxPhysicsAPI.h>
#include <task/PxCpuDispatcher.h>
#include <unordered_map>
#include <unordered_set>

#include "PhysicsFilterShader.h"
#include "RaycastHit.h"
#include "QueryTriggerInteraction.h"
#include "CollisionLayer.h"

using namespace physx;

class PhysicsComponent;
class PhysicsLayerMatrix;
struct PairHash
{
    size_t operator()(const std::pair<PhysicsComponent*, PhysicsComponent*>& p) const
    {
        return std::hash<void*>()(p.first) ^ std::hash<void*>()(p.second);
    }
};

// ----------------------------------------------------
// [ ControllerHitReport ] 
// 엔진 공용 PhysX 콜백
//  - onShapeHit : CCT ↔ PhysX Shape (RigidStatic / RigidDynamic) 충돌 시 호출
// ----------------------------------------------------
class ControllerHitReport : public PxUserControllerHitReport
{
public:
    virtual void onShapeHit(const PxControllerShapeHit& hit) override; 
    virtual void onControllerHit(const PxControllersHit&) override {}
    virtual void onObstacleHit(const PxControllerObstacleHit&) override {}
};


// ----------------------------------------------------
// [ SimulationEventCallback ] 
// 
// 각 Shape에 설정된 isTrigger에 따라서 이벤트 실행
//  - PxSimulationEventCallback 을 상속받아 구현한다. 
// ----------------------------------------------------
class SimulationEventCallback : public PxSimulationEventCallback
{
public:
    // Simulation Shape ↔ Simulation Shape
    virtual void onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs) override;
    
    // 사용 안 함
        // Trigger 이벤트는 PhysX Simulation Trigger를 사용X 
        // 모든 Trigger는 Overlap Query + PendingTriggers 방식으로 통합 처리O 
    virtual void onTrigger(PxTriggerPair* pairs, PxU32 nbPairs) override {} // Trigger Shape ↔ Simulation Shape
    virtual void onConstraintBreak(PxConstraintInfo*, PxU32) override {}
    virtual void onWake(PxActor**, PxU32) override {}
    virtual void onSleep(PxActor**, PxU32) override {}
    virtual void onAdvance(const PxRigidBody* const*, const PxTransform*, const PxU32) override {}
};

// ------------------------------
// CCT Trigger Filter (Overlap Query용)
// ------------------------------
class TriggerFilter : public PxQueryFilterCallback
{
public:
    PhysicsComponent* owner;
    TriggerFilter(PhysicsComponent* c) : owner(c) {}

    // Trigger Overlap Query 전용 필터
    //  - Trigger Shape만 통과
    //  - Layer/Mask 양방향 검사
    //  - 자기 자신(CCT Actor)은 제외
    PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags&) override;

    PxQueryHitType::Enum postFilter(
        const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*) override;
};

// ------------------------------
// CCT Collision Filter (Sweep용)
// ------------------------------
class CCTQueryFilter : public PxQueryFilterCallback 
{
public:
    PhysicsComponent* owner;
    CCTQueryFilter(PhysicsComponent* comp) : owner(comp) {}

    PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,   // CCT FilterData
        const PxShape* shape,
        const PxRigidActor* actor,
        PxHitFlags&) override;

    PxQueryHitType::Enum postFilter(
        const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*) override;
};


// ------------------------------
// Raycast Filter
// ------------------------------
class RaycastFilterCallback : public PxQueryFilterCallback
{
public:
    CollisionLayer m_RaycastLayer;
    QueryTriggerInteraction m_TriggerInteraction;

    RaycastFilterCallback(CollisionLayer layer, QueryTriggerInteraction trigger)
        : m_RaycastLayer(layer), m_TriggerInteraction(trigger) {
    }

    // Raycast 시 preFilter
    PxQueryHitType::Enum preFilter(
        const PxFilterData& filterData,
        const PxShape* shape,
        const PxRigidActor* actor,
              PxHitFlags&) override;

    PxQueryHitType::Enum postFilter(
        const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*) override;
};


// ----------------------------------------------------
// [ PhysicsSystem ] 
// 
// PhysX 기반 물리 시스템을 관리하는 싱글톤 클래스
//  - PhysX Foundation / Physics / Scene 생성 및 관리
//  - 물리 시뮬레이션 업데이트 
//  - PVD(PhysX Visual Debugger) 연결
// ----------------------------------------------------
class PhysicsSystem
{
public:
    static PhysicsSystem& Get() // 싱글톤
    {
        static PhysicsSystem instance;
        return instance;
    }

public:
    PhysicsSystem() = default;
    ~PhysicsSystem();

    bool Initialize();
    void Simulate(float dt); // 물리 시뮬레이션 1프레임 수행 
    void Shutdown();

    PxPhysics* GetPhysics() const { return m_Physics; }
    PxScene* GetScene()   const { return m_Scene; }
    PxMaterial* GetDefaultMaterial() const { return m_DefaultMaterial; }
    PxControllerManager* GetControllerManager() const { return m_ControllerManager; }


    // Actor/CCT <-> Component 매핑
    std::unordered_map<PxActor*, PhysicsComponent*> m_ActorMap;
    std::unordered_map<PxController*, PhysicsComponent*> m_CCTMap;

    std::unordered_set<std::pair<PhysicsComponent*, PhysicsComponent*>, PairHash> m_TriggerCurr;
    std::unordered_set<std::pair<PhysicsComponent*, PhysicsComponent*>, PairHash> m_TriggerPrev;

    void ResolveTriggerEvents();

    
public:
    void RegisterComponent(PxRigidActor* actor, PhysicsComponent* comp);
    void RegisterComponent(PxController* cct, PhysicsComponent* comp);

    void UnregisterComponent(PxActor* actor);       // Actor unregister
    void UnregisterComponent(PxController* cct);    // CCT unregister

    PhysicsComponent* GetComponent(PxActor* actor);
    PhysicsComponent* GetComponent(PxController* cct);


private:
    // ------------------------------------------------------
    // PhysX 기본 유틸 객체 
    // ------------------------------------------------------
    // PxDefaultAllocator       : PhysX 내부 메모리 할당자
    // PxDefaultErrorCallback   : PhysX 에러/경고 콜백 (콘솔 출력용)
    PxDefaultAllocator      m_Allocator;        
    PxDefaultErrorCallback  m_ErrorCallback;    

    // ------------------------------------------------------
    // PhysX 핵심 객체 
    // ------------------------------------------------------
    // PxFoundation         : PhysX의 최상위 객체 (가장 먼저 생성, 가장 나중에 파괴)
    // PxPhysics            : 물리 연산의 핵심 객체 (RigidBody, Shape 생성 담당)
    // PxScene              : 실제 물리 시뮬레이션이 수행되는 공간
    // PxMaterial           : 기본 물리 재질 (마찰, 반발력)
    // PxDefaultCpuDispatcher : CPU 멀티스레드 디스패처
    PxFoundation* m_Foundation = nullptr;           
    PxPhysics* m_Physics = nullptr;                 
    PxScene* m_Scene = nullptr;
    PxMaterial* m_DefaultMaterial = nullptr;
    PxDefaultCpuDispatcher* m_Dispatcher = nullptr;


    // ------------------------------------------------------
    // PVD (PhysX Visual Debugger) 관련
    // ------------------------------------------------------
    PxPvd* m_Pvd = nullptr;
    PxPvdTransport* m_PvdTransport = nullptr;


    // ------------------------------------------------------
    // Character Controller
    // ------------------------------------------------------
     PxControllerManager* m_ControllerManager = nullptr;
     ControllerHitReport m_ControllerHitReport;


     // -----------------------------------------------------
     SimulationEventCallback m_SimulationEventCallback;

public:
    PxController* CreateCapsuleController(
        const PxExtendedVec3& position,
        float radius,
        float height,
        float density = 10.0f
    );


    // Raycast
    bool RaycastAll(
        const PxVec3& origin,
        const PxVec3& direction,
        float maxDistance,
        std::vector<RaycastHit>& outHits,
        CollisionLayer layer,
        QueryTriggerInteraction triggerInteraction);

};

// RigidBody    -> PhysX SimulationEventCallback,
// CCT          -> HitReport + Sweep + Overlap,
// Trigger      -> Overlap + Pending → PhysicsSystem 에서 중앙 처리