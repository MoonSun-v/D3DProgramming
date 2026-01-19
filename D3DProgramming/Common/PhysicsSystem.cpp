#include "PhysicsSystem.h"
#include "PhysXUtils.h"
#include "Helper.h"
#include "PhysicsComponent.h"
#include "PhysicsLayerMatrix.h"

void ControllerHitReport::onShapeHit(const PxControllerShapeHit& hit)
{
    PhysicsComponent* cctComp = PhysicsSystem::Get().GetComponent(hit.controller);
    PhysicsComponent* otherComp = PhysicsSystem::Get().GetComponent(hit.actor);

    if (!cctComp || !otherComp)
        return;

    // -------------------------------------------------
    // 1. 레이어 필터 (완전 차단)
    // -------------------------------------------------
    if (!PhysicsLayerMatrix::CanCollide(
        cctComp->GetLayer(),
        otherComp->GetLayer()))
    {
        return;
    }

    // -------------------------------------------------
    // 2. Trigger는 CCT Collision 경로에서 완전 제외
    //  - Trigger는 Overlap Query(CheckCCTTriggers)에서만 처리한다.
    //  - 여기서 Trigger 허용하면 Collision + Trigger가 동시에 발생..
    // -------------------------------------------------
    if (otherComp->IsTrigger())
        return;

    // -------------------------------------------------
    // 3. Dynamic Actor 밀어내기
    // -------------------------------------------------
    PxRigidDynamic* actor = hit.actor->is<PxRigidDynamic>();
    if (actor)
    {
        // Controller → Actor 방향
        PxExtendedVec3 cctPos = hit.controller->getPosition();

        PxVec3 controllerPos(
            static_cast<PxReal>(cctPos.x),
            static_cast<PxReal>(cctPos.y),
            static_cast<PxReal>(cctPos.z));

        PxVec3 worldPos(
            static_cast<PxReal>(hit.worldPos.x),
            static_cast<PxReal>(hit.worldPos.y),
            static_cast<PxReal>(hit.worldPos.z));

        PxVec3 dir = worldPos - controllerPos;
        dir.y = 0.0f;

        if (dir.normalize() > 0.001f)
        {
            const PxF32 pushStrength = 5.0f;
            actor->addForce(dir * pushStrength, PxForceMode::eACCELERATION);
        }
    }

    // -------------------------------------------------
    // 4. Collision 이벤트 수집
    // -------------------------------------------------
    cctComp->m_CCTCurrContacts.insert(otherComp);
}



// -------------------------------------------------------------------------

PxQueryHitType::Enum TriggerFilter::preFilter(
    const PxFilterData& filterData,
    const PxShape* shape,
    const PxRigidActor* actor,
    PxHitFlags&)
{
    // Trigger 아닌 경우 무시
    if (!(shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE))
        return PxQueryHitType::eNONE;

    const PxFilterData& shapeData = shape->getQueryFilterData();

    // 양방향 레이어 + 마스크 검사
    if (!(filterData.word1 & shapeData.word0) || !(shapeData.word1 & filterData.word0))
    {
        return PxQueryHitType::eNONE;
    }

    // 자기 자신 제외
    if (actor == owner->m_Controller->getActor())
        return PxQueryHitType::eNONE;

    return PxQueryHitType::eTOUCH;
}

PxQueryHitType::Enum TriggerFilter::postFilter (const PxFilterData&, const PxQueryHit&, const PxShape*, const PxRigidActor*)
{
    return PxQueryHitType::eTOUCH;
}

// -------------------------------------------------------------------------

PxQueryHitType::Enum CCTQueryFilter::preFilter(
    const PxFilterData& filterData,
    const PxShape* shape,
    const PxRigidActor* actor,
    PxHitFlags&)
{
    const PxFilterData& shapeData = shape->getQueryFilterData();

    // 양방향 레이어 검사
    if (!(filterData.word1 & shapeData.word0) || !(shapeData.word1 & filterData.word0))
    {
        return PxQueryHitType::eNONE;
    }

    // Trigger는 CCT Sweep에서 완전 제외
    if (shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE)
    {
        return PxQueryHitType::eNONE;
    }

    return PxQueryHitType::eBLOCK;
}

PxQueryHitType::Enum CCTQueryFilter::postFilter(
    const PxFilterData&,
    const PxQueryHit&,
    const PxShape*,
    const PxRigidActor*)
{
    return PxQueryHitType::eNONE;
}

// -------------------------------------------------------------------------

PxQueryHitType::Enum RaycastFilterCallback::preFilter(
    const PxFilterData& filterData,
    const PxShape* shape,
    const PxRigidActor* actor,
    PxHitFlags&)
{
    if (!shape || !actor)
        return PxQueryHitType::eNONE;

    // Trigger 처리 
    bool isTrigger = shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
    if (isTrigger && m_TriggerInteraction == QueryTriggerInteraction::Ignore)
        return PxQueryHitType::eNONE;

    // Actor의 PhysicsComponent
    PhysicsComponent* comp = PhysicsSystem::Get().GetComponent(const_cast<PxRigidActor*>(actor));
    if (!comp)
    {
        return PxQueryHitType::eNONE;
    }

    // Raycast 레이어 필터링 : CanCollide 대신 안전하게 양방향 체크
    CollisionMask rayMask = PhysicsLayerMatrix::GetMask(m_RaycastLayer);    // rayMask:   Raycast가 감지할 수 있는 레이어들의 마스크
    CollisionMask actorMask = PhysicsLayerMatrix::GetMask(comp->GetLayer());// actorMask: Actor가 충돌할 수 있는 레이어들의 비트 마스크
    
    if (((rayMask & (uint32_t)comp->GetLayer()) == 0) || ((actorMask & (uint32_t)m_RaycastLayer) == 0))
    {
        OutputDebugStringW(L"[RaycastFilterCallback] Raycast 레이어 필터링 되었습니다.\n");
        return PxQueryHitType::eNONE;
    }

    //OutputDebugStringA(("Raycast preFilter: actor=" + comp->owner->GetName() +
    //    " | actorLayer=" + std::to_string((int)comp->GetLayer()) +
    //    " | rayLayer=" + std::to_string((int)m_RaycastLayer) + "\n").c_str());

    return PxQueryHitType::eBLOCK; // 유효 히트
}

PxQueryHitType::Enum RaycastFilterCallback::postFilter(
    const PxFilterData&,
    const PxQueryHit&,
    const PxShape*,
    const PxRigidActor*)
{
    return PxQueryHitType::eBLOCK;
}


// -------------------------------------------------------------------------

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
            comp->ResolveCCTTriggers(); // 수집만 진행 
        }
    }
    ResolveTriggerEvents(); // 한 번만 Trigger 해석
}


// [ 모든 Trigger 이벤트는 PhysicsSystem에서 중앙 처리 ]
// - 중복 제거
// - Enter / Stay / Exit 판정
// - 순서 안정성 보장
void PhysicsSystem::ResolveTriggerEvents()
{
    m_TriggerCurr.clear();

    // --------------------------------------------------
    // 1. 모든 PhysicsComponent에서 Trigger 수집
    // --------------------------------------------------
    for (auto it = m_ActorMap.begin(); it != m_ActorMap.end(); ++it)
    {
        PhysicsComponent* comp = it->second;

        if (!comp)
            continue;

        for (PhysicsComponent* other : comp->m_PendingTriggers)
        {
            // (A,B) == (B,A) 정규화
            PhysicsComponent* a = comp < other ? comp : other;
            PhysicsComponent* b = comp < other ? other : comp;

            m_TriggerCurr.insert(std::make_pair(a, b));
        }

        comp->m_PendingTriggers.clear();
    }

    // --------------------------------------------------
    // 2. Trigger Enter / Stay
    // --------------------------------------------------
    for (const auto& pair : m_TriggerCurr)
    {
        if (m_TriggerPrev.find(pair) == m_TriggerPrev.end())
        {
            pair.first->OnTriggerEnter(pair.second);
            pair.second->OnTriggerEnter(pair.first);
        }
        else
        {
            pair.first->OnTriggerStay(pair.second);
            pair.second->OnTriggerStay(pair.first);
        }
    }

    // --------------------------------------------------
    // 3. Trigger Exit
    // --------------------------------------------------
    for (const auto& pair : m_TriggerPrev)
    {
        if (m_TriggerCurr.find(pair) == m_TriggerCurr.end())
        {
            pair.first->OnTriggerExit(pair.second);
            pair.second->OnTriggerExit(pair.first);
        }
    }

    m_TriggerPrev = std::move(m_TriggerCurr);
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
            // CCT Shape은 Simulation에서 제거함. 
            // - RigidBody와 직접 충돌 계산하지 않음
            // - 모든 충돌은 Sweep(Query) 기반
            shapes[i]->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false/*true*/);
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
    auto* compA = PhysicsSystem::Get().GetComponent(pairHeader.actors[0]);
    auto* compB = PhysicsSystem::Get().GetComponent(pairHeader.actors[1]);

    if (!compA || !compB)
        return;

    if (!PhysicsLayerMatrix::CanCollide(
        compA->GetLayer(),
        compB->GetLayer()))
    {
        return;
    }

    for (PxU32 i = 0; i < nbPairs; i++)
    {
        const PxContactPair& pair = pairs[i];

        if (pair.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
        {
            compA->OnCollisionEnter(compB);
            compB->OnCollisionEnter(compA);
        }

        if (pair.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS)
        {
            compA->OnCollisionStay(compB);
            compB->OnCollisionStay(compA);
        }

        if (pair.events & PxPairFlag::eNOTIFY_TOUCH_LOST)
        {
            compA->OnCollisionExit(compB);
            compB->OnCollisionExit(compA);
        }
    }
}



// ----------------------------------------------------
// [ Raycast ] 
// ----------------------------------------------------

bool PhysicsSystem::RaycastAll(
    const PxVec3& origin,
    const PxVec3& direction,
    float maxDistance,
    std::vector<RaycastHit>& outHits,
    CollisionLayer layer,
    QueryTriggerInteraction triggerInteraction)
{
    outHits.clear();
    if (!m_Scene) return false;

    const PxU32 maxHits = 128;
    PxRaycastHit hitArray[maxHits];
    PxRaycastBuffer hitBuffer(hitArray, maxHits);

    PxQueryFilterData filterData;
    filterData.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC | PxQueryFlag::ePREFILTER;
    filterData.data.word0 = static_cast<PxU32>(layer);
    filterData.data.word1 = PhysicsLayerMatrix::GetMask(layer);

    RaycastFilterCallback filterCallback(layer, triggerInteraction);
    PxHitFlags hitFlags = PxHitFlag::eDEFAULT | PxHitFlag::eMESH_MULTIPLE;

    bool bHit = m_Scene->raycast(origin, direction.getNormalized(), maxDistance, hitBuffer, hitFlags, filterData, &filterCallback);
    if (!bHit) return false;

    auto ProcessHit = [&](const PxRaycastHit& pxHit) -> bool
        {
            PxShape* shape = pxHit.shape;
            PhysicsComponent* comp = GetComponent(pxHit.actor);
            if (!comp) return false;

            bool isTrigger = shape->getFlags() & PxShapeFlag::eTRIGGER_SHAPE;
            if (isTrigger && triggerInteraction == QueryTriggerInteraction::Ignore)
                return false;

            if (!PhysicsLayerMatrix::CanCollide(layer, comp->GetLayer()))
                return false;

            RaycastHit hitInfo;
            hitInfo.component = comp;
            hitInfo.point = pxHit.position;
            hitInfo.normal = pxHit.normal;
            hitInfo.distance = pxHit.distance;
            hitInfo.shape = shape;
            hitInfo.actor = pxHit.actor;

            outHits.push_back(hitInfo);
            return true;
        };

    if (hitBuffer.hasBlock) ProcessHit(hitBuffer.block);
    for (PxU32 i = 0; i < hitBuffer.getNbAnyHits(); ++i)
        ProcessHit(hitBuffer.getAnyHit(i));

    return !outHits.empty();
}