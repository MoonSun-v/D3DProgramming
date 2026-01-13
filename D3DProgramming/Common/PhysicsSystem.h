#pragma once
#include <PxPhysicsAPI.h>
#include <task/PxCpuDispatcher.h>

using namespace physx;

// ----------------------------------------------------
// [ ControllerHitReport ] 
// 엔진 공용 PhysX 콜백
//  - Character Controller가 Dynamic Actor를 밀어내기 위한 콜백
// ----------------------------------------------------
class ControllerHitReport : public PxUserControllerHitReport
{
public:
    virtual void onShapeHit(const PxControllerShapeHit& hit) override;
    virtual void onControllerHit(const PxControllersHit&) override {}
    virtual void onObstacleHit(const PxControllerObstacleHit&) override {}
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
    void Shutdown();

    // 물리 시뮬레이션 1프레임 수행 
    void Simulate(float dt);

    PxPhysics* GetPhysics() const { return m_Physics; }
    PxScene* GetScene()   const { return m_Scene; }
    PxMaterial* GetDefaultMaterial() const { return m_DefaultMaterial; }
    // PxControllerManager* GetControllerManager() const { return m_ControllerManager; }

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
     //PxControllerManager* m_ControllerManager = nullptr;
     //ControllerHitReport m_ControllerHitReport;


public:
    //PxController* CreateCapsuleController(
    //    const PxExtendedVec3& position,
    //    float radius,
    //    float height,
    //    float density = 10.0f
    //);
};