#include "PhysicsSystem.h"
#include "PhysXUtils.h"
#include "Helper.h"
#include "PhysicsComponent.h"
#include "PhysicsLayerMatrix.h"

void ControllerHitReport::onShapeHit(const PxControllerShapeHit& hit)
{
    // -----------------------------
    // 1. Dynamic Actor와 충돌했을 때 밀어내기
    // -----------------------------
    PxRigidDynamic* actor = hit.actor->is<PxRigidDynamic>();
    if (actor)
    {
        // 충돌 방향 계산 (Controller에서 Actor로)
        PxExtendedVec3 cctPos = hit.controller->getPosition();
        PxVec3 worldPos((PxReal)hit.worldPos.x, (PxReal)hit.worldPos.y, (PxReal)hit.worldPos.z);
        PxVec3 controllerPos((PxReal)cctPos.x, (PxReal)cctPos.y, (PxReal)cctPos.z);
        PxVec3 dir = worldPos - controllerPos;
        dir.y = 0.0f;

        const PxF32 dirMagnitude = dir.magnitude();
        if (dirMagnitude > 0.001f)
        {
            dir.normalize();

            // 간단하고 고정된 힘으로 밀기 (PhysX 예제 방식)
            const PxF32 pushStrength = 5.0f;
            actor->addForce(dir * pushStrength, PxForceMode::eACCELERATION);
        }
    }

    // -----------------------------
    // 2. 이벤트 처리 (Collision)
    // -----------------------------
    PhysicsComponent* cctComp = PhysicsSystem::Get().GetComponent(hit.controller);
    PhysicsComponent* otherComp = PhysicsSystem::Get().GetComponent(hit.actor);

    if (!cctComp || !otherComp) return;

    // 충돌한 Shape
    PxShape* shape = hit.shape;

    bool isTrigger = shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;

    // 이번 프레임에 닿았다는 사실 기록
    if (isTrigger)
    {
        // CCT Trigger
        cctComp->m_CCTCurrTriggers.insert(otherComp);
    }
    else
    {
        // CCT Collision (기본)
        cctComp->m_CCTCurrContacts.insert(otherComp);
    }
}


PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

bool PhysicsSystem::Initialize()
{
    // ------------------------------------------------------
    // 1. Foundation : PhysX의 모든 객체는 Foundation 위에서 동작
    // ------------------------------------------------------
    m_Foundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_Allocator, m_ErrorCallback);
    if (!m_Foundation)
    {
        LOG_ERRORA("PxCreateFoundation failed!");
        return false;
    }


    // ------------------------------------------------------
    // 2. PVD
    // ------------------------------------------------------
    m_Pvd = PxCreatePvd(*m_Foundation);
    m_PvdTransport = PxDefaultPvdSocketTransportCreate( // 로컬 PC의 PVD 프로그램과 소켓으로 연결 
        "127.0.0.1", // IP
        5425,        // 포트 (PVD 기본값)
        10           // 연결 타임아웃(ms)
    );
    m_Pvd->connect(*m_PvdTransport, PxPvdInstrumentationFlag::eALL); // 모든 디버깅 데이터 전송 


    // ------------------------------------------------------
    // 3. Physics 객체 생성 
    // ------------------------------------------------------
    m_Physics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *m_Foundation,
        PxTolerancesScale(), // 길이/질량 기준 스케일
        true,                // PhysX Extensions 사용
        m_Pvd                // PVD 연결
    );
    if (!m_Physics)
    {
        LOG_ERRORA("PxCreatePhysics failed!");
        return false;
    }


    // ------------------------------------------------------
    // 4. Scene 생성 
    // ------------------------------------------------------
    PxSceneDesc sceneDesc(m_Physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f); // 중력 설정 (Y축 아래 방향)

    sceneDesc.simulationEventCallback = &m_SimulationEventCallback;
    m_Dispatcher = PxDefaultCpuDispatcherCreate(2); // CPU 물리 연산을 담당할 스레드 풀 (2 스레드)
    sceneDesc.cpuDispatcher = m_Dispatcher;
    sceneDesc.filterShader = PhysicsFilterShader;
    // sceneDesc.filterShader = PxDefaultSimulationFilterShader;
    
    m_Scene = m_Physics->createScene(sceneDesc);
    if (!m_Scene)
        return false;


    // ------------------------------------------------------
    // 5. PVD Scene 설정 (있어야 충돌/접촉 보임)
    // ------------------------------------------------------
    if (PxPvdSceneClient* client = m_Scene->getScenePvdClient())
    {
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }


    // ------------------------------------------------------
    // 6. Default Material
    // ------------------------------------------------------
    m_DefaultMaterial = m_Physics->createMaterial(
        0.5f, // 정지 마찰력 (static friction)
        0.5f, // 동적 마찰력 (dynamic friction)
        0.6f  // 반발력 (restitution)
    );


    // ------------------------------------------------------
    // 7. Character Controller Manager
    // ------------------------------------------------------
    m_ControllerManager = PxCreateControllerManager(*m_Scene);
    if (!m_ControllerManager)
    {
        LOG_ERRORA("PxCreateControllerManager failed!");
        return false;
    }

    PhysicsLayerMatrix::Initialize();

    return true;
}


void PhysicsSystem::Simulate(float dt)
{
    if (!m_Scene)
        return;

    m_Scene->simulate(dt);      // 물리 연산 요청 (비동기)
    m_Scene->fetchResults(true); // 결과가 끝날 때까지 대기 후 적용

    for (auto& it : m_CCTMap)
    {
        PhysicsComponent* comp = it.second;
        if (comp)
        {
            comp->ResolveCCTCollisions();
            comp->CheckCCTTriggers();   // CCT 위치 기반 Overlap Query
            comp->ResolveCCTTriggers();
        }
    }
}


PxController* PhysicsSystem::CreateCapsuleController(
    const PxExtendedVec3& position,
    float radius,
    float height,
    float density)
{
    if (!m_ControllerManager || !m_DefaultMaterial)
        return nullptr;

    PxCapsuleControllerDesc desc;
    desc.position = position;
    desc.radius = radius;       
    desc.height = height;
    desc.material = m_DefaultMaterial;
    desc.density = density;
    desc.climbingMode = PxCapsuleClimbingMode::eEASY; // 계단 처리 방식 
    desc.slopeLimit = cosf(PxPi / 4);   // 오를 수 있는 최대 경사 : 45도
    desc.stepOffset = 0.5f;             // 자동으로 넘을 수 있는 턱
    desc.contactOffset = 0.1f;          // 충돌 여유 (떨림 방지)
    desc.reportCallback = &m_ControllerHitReport;

    PxController* controller = m_ControllerManager->createController(desc);
    if (!controller)
        return nullptr;

    // Controller Actor 충돌 설정
    if (PxRigidDynamic* actor = controller->getActor())
    {
        PxShape* shapes[8];
        PxU32 count = actor->getShapes(shapes, 8);
        for (PxU32 i = 0; i < count; ++i)
        {
            shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
            shapes[i]->setFlag(PxShapeFlag::eSCENE_QUERY_SHAPE, true);
        }
    }

    return controller;
}

void PhysicsSystem::RegisterComponent(PxRigidActor* actor, PhysicsComponent* comp)
{
    if (actor) m_ActorMap[actor] = comp;
}
void PhysicsSystem::RegisterComponent(PxController* cct, PhysicsComponent* comp)
{
    if (cct) m_CCTMap[cct] = comp;
}

void PhysicsSystem::UnregisterComponent(PxActor* actor)
{
    if (!actor) return;
    m_ActorMap.erase(actor);
}
void PhysicsSystem::UnregisterComponent(PxController* cct)
{
    if (!cct) return;
    m_CCTMap.erase(cct);
}

PhysicsComponent* PhysicsSystem::GetComponent(PxActor* actor)
{
    auto it = m_ActorMap.find(actor);
    return (it != m_ActorMap.end()) ? it->second : nullptr;
}
PhysicsComponent* PhysicsSystem::GetComponent(PxController* cct)
{
    auto it = m_CCTMap.find(cct);
    return (it != m_CCTMap.end()) ? it->second : nullptr;
}


void PhysicsSystem::Shutdown()
{
    PX_RELEASE(m_ControllerManager);
    PX_RELEASE(m_Scene);
    PX_RELEASE(m_Dispatcher);
    PX_RELEASE(m_DefaultMaterial);
    PX_RELEASE(m_Physics);

    if (m_Pvd) m_Pvd->disconnect();

    PX_RELEASE(m_PvdTransport);
    PX_RELEASE(m_Pvd);
    PX_RELEASE(m_Foundation);
}


// ----------------------------------------------------
// [ Simulation Event Callback ] 
// ----------------------------------------------------

void SimulationEventCallback::onContact(
    const PxContactPairHeader& pairHeader,
    const PxContactPair* pairs,
    PxU32 nbPairs)
{
    auto compA = PhysicsSystem::Get().GetComponent(pairHeader.actors[0]);
    auto compB = PhysicsSystem::Get().GetComponent(pairHeader.actors[1]);

    if (!compA || !compB) return;

    for (PxU32 i = 0; i < nbPairs; i++)
    {
        const PxContactPair& pair = pairs[i];

        if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            compA->OnCollisionEnter(compB);
            compB->OnCollisionEnter(compA);

            compA->m_CollisionActors.insert(compB);
            compB->m_CollisionActors.insert(compA);
        }
        else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
        {
            compA->OnCollisionStay(compB);
            compB->OnCollisionStay(compA);
        }
        else if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            compA->OnCollisionExit(compB);
            compB->OnCollisionExit(compA);

            compA->m_CollisionActors.erase(compB);
            compB->m_CollisionActors.erase(compA);
        }
    }
}


void SimulationEventCallback::onTrigger(PxTriggerPair* pairs, PxU32 count)
{
    for (PxU32 i = 0; i < count; i++)
    {
        PxTriggerPair& pair = pairs[i];
        auto compA = PhysicsSystem::Get().GetComponent(pair.triggerActor);
        auto compB = PhysicsSystem::Get().GetComponent(pair.otherActor);

        if (!compA || !compB) continue;

        if (pair.status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            compA->OnTriggerEnter(compB);
            compB->OnTriggerEnter(compA);

            compA->m_TriggerActors.insert(compB);
            compB->m_TriggerActors.insert(compA);
        }
        else if (pair.status & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
        {
            compA->OnTriggerStay(compB);
            compB->OnTriggerStay(compA);
        }
        else if (pair.status & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            compA->OnTriggerExit(compB);
            compB->OnTriggerExit(compA);

            compA->m_TriggerActors.erase(compB);
            compB->m_TriggerActors.erase(compA);
        }
    }
}
