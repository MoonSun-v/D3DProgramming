#include "PhysicsComponent.h"
#include "PhysicsSystem.h"
#include "PhysXUtils.h"
#include "Transform.h"

PhysicsComponent::~PhysicsComponent()
{
    PX_RELEASE(m_Actor);
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
    OutputDebugStringA("[Physics] CreateCollider called\n");

    auto& phys = PhysicsSystem::Get();
    PxPhysics* px = phys.GetPhysics();
    PxMaterial* mat = phys.GetDefaultMaterial();


    // ----------------------
    // Shape 생성
    // ----------------------
    PxTransform localPose;
    localPose.p = ToPx(d.localOffset);
    localPose.q = ToPxQuat(XMLoadFloat4(&d.localRotation));

    switch (collider)
    {
    case ColliderType::Box:
        m_Shape = px->createShape(PxBoxGeometry(d.halfExtents.x, d.halfExtents.y, d.halfExtents.z),*mat);
        break;

    case ColliderType::Sphere:
        m_Shape = px->createShape(PxSphereGeometry(d.radius),*mat);
        break;

    case ColliderType::Capsule:
        m_Shape = px->createShape(PxCapsuleGeometry(d.radius, d.height * 0.5f),*mat);
    
        PxQuat capsuleRot(PxHalfPi, PxVec3(0, 0, 1));// X축 캡슐 → Y축 캡슐로 회전 // Z축 +90도
        localPose.q = capsuleRot * localPose.q;
        break;
    }
    m_Shape->setLocalPose(localPose);



    // ----------------------
    // Actor 생성
    // ----------------------
    if (body == PhysicsBodyType::Static)
    {
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
    // 연결 & 질량
    // ----------------------
    m_Actor->attachShape(*m_Shape);

    if (body == PhysicsBodyType::Dynamic)
    {
        PxRigidBodyExt::updateMassAndInertia( // 질량 계산 : Shape 부피 × density
            *static_cast<PxRigidDynamic*>(m_Actor),
            d.density
        );
    }

    phys.GetScene()->addActor(*m_Actor); // 물리 씬에 추가 

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

    PxTransform px;
    px.p = ToPx(transform->position);
    px.q = ToPxQuat(XMLoadFloat4(&transform->rotation)); // Quaternion 그대로

    m_Actor->setGlobalPose(px);
}

// Physics → Transform (물리 시뮬 하고나서 매 프레임 실행)
void PhysicsComponent::SyncFromPhysics()
{
    if (!transform) return;

    if (m_Controller)
    {
        PxExtendedVec3 p = m_Controller->getPosition();

        // [ Offset ] offset을 다시 빼서 Transform 위치 복원
        transform->position = {
            (float)p.x - m_ControllerOffset.x,
            (float)p.y - m_ControllerOffset.y,
            (float)p.z - m_ControllerOffset.z
        };
        // 회전은 Transform 유지
    }

    if (m_Actor)
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
    OutputDebugStringA("[Physics] CreateCharacterCapsule called\n");

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
        transform->position.x + localOffset.x,
        transform->position.y + localOffset.y,
        transform->position.z + localOffset.z
    );

    m_Controller = phys.CreateCapsuleController(
        pos,
        radius,
        height,
        10.0f   // density (사실상 무의미) density는 반드시 > 0
    );
}

void PhysicsComponent::MoveCharacter(
    const Vector3& input,
    float deltaTime)
{
    if (!m_Controller) return;

    // --------------------
    // 입력 이동
    // --------------------
    PxVec3 move(input.x, 0, input.z);
    if (move.magnitudeSquared() > 0)
        move.normalize();

    move *= 6.0f * deltaTime;

    // --------------------
    // Controller 상태 얻기
    // --------------------
    PxControllerState state;
    m_Controller->getState(state);

    // --------------------
    // 중력 처리
    // --------------------
    static float verticalVel = 0.0f;

    bool isGrounded =
        state.collisionFlags & PxControllerCollisionFlag::eCOLLISION_DOWN;

    if (isGrounded)
    {
        if (verticalVel < 0)
            verticalVel = 0.0f;

        // 바닥 접촉 유지용 미세 하강
        move.y = -0.01f;
    }
    else
    {
        verticalVel += -9.8f * deltaTime;
        move.y = verticalVel * deltaTime;
    }

    // --------------------
    // 이동
    // --------------------
    PxControllerFilters filters;
    m_Controller->move(move, 0.01f, deltaTime, filters);
}
