#include "PhysicsComponent.h"
#include "PhysicsSystem.h"
#include "PhysXUtils.h"
#include "Transform.h"
#include "PhysicsLayerMatrix.h"

PhysicsComponent::~PhysicsComponent()
{
    if (m_Controller)
    {
        PhysicsSystem::Get().UnregisterComponent(m_Controller);
        PX_RELEASE(m_Controller);
    }
    if (m_Actor)
    {
        PhysicsSystem::Get().UnregisterComponent(m_Actor);
        PX_RELEASE(m_Actor);
    }
}

// ------------------------------
// 외부 API 
// - density : 질량 
// ------------------------------

// Box 
void PhysicsComponent::CreateStaticBox(const Vector3& half, const Vector3& localOffset)
{
    ColliderDesc d;
    d.halfExtents = half;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Box, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateTriggerBox(const Vector3& half, const Vector3& localOffset)
{
    ColliderDesc d;
    d.halfExtents = half;
    d.localOffset = localOffset;
    d.isTrigger = true;

    CreateCollider(ColliderType::Box, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateDynamicBox(const Vector3& half, float density, const Vector3& localOffset)
{
    ColliderDesc d;
    d.halfExtents = half;
    d.density = density;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Box, PhysicsBodyType::Dynamic, d);
}


// Sphere 
void PhysicsComponent::CreateStaticSphere(float radius, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Sphere, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateDynamicSphere(float radius, float density, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.density = density;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Sphere, PhysicsBodyType::Dynamic, d);
}

// Capsule 
void PhysicsComponent::CreateStaticCapsule(float radius, float height, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.height = height;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Capsule, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateTriggerCapsule(float radius, float height, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.height = height;
    d.localOffset = localOffset;
    d.isTrigger = true;

    CreateCollider(ColliderType::Capsule, PhysicsBodyType::Static, d);
}
void PhysicsComponent::CreateDynamicCapsule(float radius, float height, float density, const Vector3& localOffset)
{
    ColliderDesc d;
    d.radius = radius;
    d.height = height;
    d.density = density;
    d.localOffset = localOffset;
    CreateCollider(ColliderType::Capsule, PhysicsBodyType::Dynamic, d);
}


// ------------------------------
// 내부 생성 
// ------------------------------
void PhysicsComponent::CreateCollider(ColliderType collider, PhysicsBodyType body, const ColliderDesc& d)
{
    auto& phys = PhysicsSystem::Get();
    PxPhysics* px = phys.GetPhysics();
    PxMaterial* mat = phys.GetDefaultMaterial();

    m_IsTrigger = d.isTrigger;

    // ----------------------
    // Shape 생성
    // ----------------------
    PxTransform localPose;
    localPose.p = ToPx(d.localOffset);
    localPose.q = ToPxQuat(XMLoadFloat4(&d.localRotation));

    switch (collider)
    {
    case ColliderType::Box:
        m_Shape = px->createShape(PxBoxGeometry(d.halfExtents.x * WORLD_TO_PHYSX, d.halfExtents.y * WORLD_TO_PHYSX, d.halfExtents.z * WORLD_TO_PHYSX), *mat, true);
        // m_Shape = px->createShape(PxBoxGeometry(0.5f, 0.5f, 0.5f), *mat, true);
        break;

    case ColliderType::Sphere:
        m_Shape = px->createShape(PxSphereGeometry(d.radius * WORLD_TO_PHYSX),*mat, true);
        break;

    case ColliderType::Capsule:
        m_Shape = px->createShape(PxCapsuleGeometry(d.radius * WORLD_TO_PHYSX, (d.height * 0.5f) * WORLD_TO_PHYSX),*mat, true);
    
        PxQuat capsuleRot(PxHalfPi, PxVec3(0, 0, 1));// X축 캡슐 → Y축 캡슐로 회전 // Z축 +90도
        localPose.q = capsuleRot * localPose.q;
        break;
    }
    m_Shape->setLocalPose(localPose);


    // ----------------------
    // Shape Flag (Trigger / Simulation)
    // ----------------------
    if (d.isTrigger)
    {
        // Trigger는 충돌 계산 X
        m_Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        m_Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
    }
    else
    {
        // 일반 Collider
        m_Shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
        m_Shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
    }


    // ----------------------
    // Actor 생성
    // ----------------------
    if (body == PhysicsBodyType::Static || d.isTrigger)
    {
        // Trigger는 무조건 Static 
        m_Actor = px->createRigidStatic(PxTransform(PxIdentity));
    }
    else
    {
        PxRigidDynamic* dyn = px->createRigidDynamic(PxTransform(PxIdentity));

        if (body == PhysicsBodyType::Kinematic)
        {
            dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
        }

        m_Actor = dyn;
    }

    // ----------------------
    // Shape 연결
    // ----------------------
     m_Actor->attachShape(*m_Shape);


    // ----------------------
    // 질량 계산
    // ----------------------
    if (body == PhysicsBodyType::Dynamic)
    {
        PxRigidBodyExt::updateMassAndInertia( // 질량 계산 : Shape 부피 × density
            *static_cast<PxRigidDynamic*>(m_Actor),
            d.density
        );
    }

    phys.GetScene()->addActor(*m_Actor); // 물리 씬에 추가 
    phys.RegisterComponent(m_Actor, this);

    m_BodyType = body;
    m_ColliderType = collider;
}


// ------------------------------
// 좌표 변환
// ------------------------------

// Transform → Physics  
//                      **  Dynamic에는 매 프레임 쓰면 안 됨
//                      *** Controller는 여기서 처리 안 함 (캐릭터에 쓰면 안됨)
void PhysicsComponent::SyncToPhysics()
{
    if (m_Controller) return;
    if (!m_Actor || !transform) return;

    ApplyFilter(); // PhysX 오브젝트가 “완성된 시점” 에 호출해야함. (엔진 합치면서 변경될듯) 

    PxTransform px;
    px.p = ToPx(transform->position);
    px.q = ToPxQuat(XMLoadFloat4(&transform->rotation)); // Quaternion 그대로

    m_Actor->setGlobalPose(px);
}

// Physics → Transform (물리 시뮬 하고나서 매 프레임 실행)
void PhysicsComponent::SyncFromPhysics()
{
    if (!transform) return;

    if (m_Controller) // CCT 
    {
        PxExtendedVec3 p = m_Controller->getPosition();

        // PhysX(m) → Render(cm)
        XMFLOAT3 posCM = {
            (float)p.x * PHYSX_TO_WORLD,
            (float)p.y * PHYSX_TO_WORLD,
            (float)p.z * PHYSX_TO_WORLD
        };

        // [ Offset ] offset을 다시 빼서 Transform 위치 복원
        transform->position = {
            posCM.x - m_ControllerOffset.x, // offset은 cm
            posCM.y - m_ControllerOffset.y,
            posCM.z - m_ControllerOffset.z
        };
        // 회전은 Transform 유지
    }
    else if (m_Actor) // 일반 액터 
    {
        PxTransform px = m_Actor->getGlobalPose();
        transform->position = ToDX(px.p);       // 위치 변환
        transform->rotation = ToDXQuatF4(px.q); // Quaternion 그대로
    }
}


// ------------------------------
// CCT (Character Controller)
// ------------------------------
void PhysicsComponent::CreateCharacterCapsule(float radius, float height, const Vector3& localOffset)
{
    if (m_Actor)
    {
        PhysicsSystem::Get().GetScene()->removeActor(*m_Actor);
        PX_RELEASE(m_Actor);
        m_Actor = nullptr;
    }

    auto& phys = PhysicsSystem::Get();

    m_ControllerOffset = localOffset;

    // [ Offset ] PhysX에는 offset 없는 위치 전달 -> SyncFromPhysics에서 반대로 보정할 예정 
    PxExtendedVec3 pos(
        (transform->position.x + localOffset.x) * WORLD_TO_PHYSX,
        (transform->position.y + localOffset.y) * WORLD_TO_PHYSX,
        (transform->position.z + localOffset.z) * WORLD_TO_PHYSX
    );

    m_Controller = phys.CreateCapsuleController(
        pos,
        radius * WORLD_TO_PHYSX,
        height * WORLD_TO_PHYSX,
        10.0f   // density (사실상 무의미) density는 반드시 > 0
    );

    PhysicsSystem::Get().RegisterComponent(m_Controller, this);

    // CCT 생성 후 레이어 강제 적용
    SetLayer(m_Layer);
}

void PhysicsComponent::MoveCharacter(const Vector3& wishDir, float fixedDt)
{
    if (!m_Controller) return;

    // --------------------
    // 1. 수평 이동속도 (m/s) + 입력 방향 (정규화, PhysX 기준)
    // --------------------
    PxVec3 velocity(0, 0, 0);

    if (wishDir.LengthSquared() > 0.0f)
    {
        PxVec3 dir(wishDir.x, 0, wishDir.z);
        dir.normalize();
        velocity.x = dir.x * m_MoveSpeed;
        velocity.z = dir.z * m_MoveSpeed;
    }

    // --------------------
    // 2. 지면 체크 
    // --------------------
    PxControllerState state;
    m_Controller->getState(state);

    bool isGrounded = state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN;

    // --------------------
    // 3. 점프 처리
    // --------------------
    if (isGrounded)
    {
        if (isGrounded && m_RequestJump)
        {
            m_VerticalVelocity = m_JumpSpeed;
            m_RequestJump = false;
        }
        else if (m_VerticalVelocity < 0.0f)
        {
            m_VerticalVelocity = m_MinDown; // 바닥에 밀착
        }
    }
    else
    {
        // 공중 중력
        m_VerticalVelocity += -9.8f * fixedDt;
    }

    velocity.y = m_VerticalVelocity;

    // --------------------
    // 4. 이동 거리
    // --------------------
    PxVec3 move = velocity * fixedDt;

    // --------------------
    // 5. 필터
    // --------------------
    CCTQueryFilter queryFilter(this);
    PxControllerFilters filters(&m_CCTFilterData, &queryFilter, nullptr);

    // --------------------
    // 6. 이동
    // --------------------
    m_Controller->move(
        move,
        0.01f,
        fixedDt,
        filters
    );
}

void PhysicsComponent::Jump()
{
    if (!m_Controller || m_RequestJump)
        return;
    OutputDebugString((L"[PhysicsComponent] : Jump() 실행 \n"));
    m_RequestJump = true; 
}

void PhysicsComponent::ResolveCCTCollisions()
{
    // Enter / Stay
    for (auto* other : m_CCTCurrContacts)
    {
        if (m_CCTPrevContacts.find(other) == m_CCTPrevContacts.end())
        {
            OnCollisionEnter(other);
            other->OnCollisionEnter(this);
        }
        else
        {
            OnCollisionStay(other);
            other->OnCollisionStay(this);
        }
    }

    // Exit
    for (auto* other : m_CCTPrevContacts)
    {
        if (m_CCTCurrContacts.find(other) == m_CCTCurrContacts.end())
        {
            OnCollisionExit(other);
            other->OnCollisionExit(this);
        }
    }

    // 다음 프레임 준비
    m_CCTPrevContacts = std::move(m_CCTCurrContacts);
    m_CCTCurrContacts.clear();
}

void PhysicsComponent::ResolveCCTTriggers() 
{
    // Enter / Stay
    for (auto* other : m_CCTCurrTriggers)
    {
        if (m_CCTPrevTriggers.find(other) == m_CCTPrevTriggers.end())
        {
            OnTriggerEnter(other);
            other->OnTriggerEnter(this);
        }
        else
        {
            OnTriggerStay(other);
            other->OnTriggerStay(this);
        }
    }

    // Exit
    for (auto* other : m_CCTPrevTriggers)
    {
        if (m_CCTCurrTriggers.find(other) == m_CCTCurrTriggers.end())
        {
            OnTriggerExit(other);
            other->OnTriggerExit(this);
        }
    }

    m_CCTPrevTriggers = std::move(m_CCTCurrTriggers);
    m_CCTCurrTriggers.clear();
}

void PhysicsComponent::CheckCCTTriggers() 
{
    if (!m_Controller)
        return;

    PxScene* scene = PhysicsSystem::Get().GetScene();

    // -------------------------------------------------
    // 1. CCT Capsule 정보
    // -------------------------------------------------
    PxCapsuleController* capsuleCtrl = static_cast<PxCapsuleController*>(m_Controller);
    const float radius = capsuleCtrl->getRadius();
    const float height = capsuleCtrl->getHeight(); // cylinder height
    const float shrink = 0.01f;

    PxCapsuleGeometry capsule(
        PxMax(0.0f, radius - shrink),
        PxMax(0.0f, (height * 0.5f) - shrink)
    );


    // -------------------------------------------------
    // 2. CCT 위치 (PhysX 기준)
    // -------------------------------------------------
    PxExtendedVec3 p = m_Controller->getPosition();
    PxTransform pose(PxVec3((float)p.x, (float)p.y, (float)p.z));



    // -------------------------------------------------
    // 3. Overlap Query용 필터
    // -------------------------------------------------
    TriggerFilter filter(this);

    PxOverlapBufferN<64> hit;
    PxQueryFilterData qfd;
    qfd.data = m_CCTFilterData;
    qfd.flags = PxQueryFlag::eSTATIC | PxQueryFlag::eDYNAMIC;

    scene->overlap(capsule, pose, hit, qfd, &filter);



    // -------------------------------------------------
    // 4. Trigger 수집
    // -------------------------------------------------
    m_CCTCurrTriggers.clear();
    for (PxU32 i = 0; i < hit.getNbAnyHits(); i++)
    {
        PhysicsComponent* comp = PhysicsSystem::Get().GetComponent(hit.getAnyHit(i).actor);
        if (!comp)
            continue;

        // Trigger가 아니면 수집하지 않음
        if (!comp->IsTrigger())
            continue;

        // 레이어 필터 
        if (!PhysicsLayerMatrix::CanCollide(m_Layer, comp->GetLayer()))
            continue;

        m_CCTCurrTriggers.insert(comp);
    }
}

void PhysicsComponent::SetLayer(CollisionLayer layer)
{
    m_Layer = layer;
    m_Mask = PhysicsLayerMatrix::GetMask(layer);

    ApplyFilter();
}


void PhysicsComponent::ApplyFilter()
{
    // ----------------------------
    // RigidActor / Shape 용
    // ----------------------------
    if (m_Shape)
    {
        PxFilterData data;
        data.word0 = (uint32_t)m_Layer;
        data.word1 = PhysicsLayerMatrix::GetMask(m_Layer);
        data.word2 = 0;
        data.word3 = 0;

        m_Shape->setSimulationFilterData(data);
        m_Shape->setQueryFilterData(data);
    }

    // ----------------------------
    // CCT 전용 Query Filter
    // ----------------------------
    if (m_Controller)
    {
        m_CCTFilterData.word0 = (uint32_t)m_Layer;          // 자기 레이어
        m_CCTFilterData.word1 = PhysicsLayerMatrix::GetMask(m_Layer);
        m_CCTFilterData.word2 = 0;
        m_CCTFilterData.word3 = 0;
    }
}
