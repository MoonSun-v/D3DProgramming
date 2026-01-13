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
    auto& phys = PhysicsSystem::Get();
    PxPhysics* px = phys.GetPhysics();
    PxMaterial* mat = phys.GetDefaultMaterial();

    PxTransform localPose;
    localPose.p = ToPx(d.localOffset);
    localPose.q = ToPxQuat(XMLoadFloat4(&d.localRotation));

    // [ Shape ]
    switch (collider)
    {
    case ColliderType::Box:
        m_Shape = px->createShape(PxBoxGeometry(d.halfExtents.x, d.halfExtents.y, d.halfExtents.z),*mat);
        m_Shape->setLocalPose(localPose);
        break;

    case ColliderType::Sphere:
        m_Shape = px->createShape(PxSphereGeometry(d.radius),*mat);
        m_Shape->setLocalPose(localPose);
        break;

    case ColliderType::Capsule:
        m_Shape = px->createShape(PxCapsuleGeometry(d.radius, d.height * 0.5f),*mat);
    
        PxQuat capsuleRot(PxHalfPi, PxVec3(0, 0, 1));// X축 캡슐 → Y축 캡슐로 회전 // Z축 +90도
        localPose.q = capsuleRot * localPose.q;
        m_Shape->setLocalPose(localPose);
        break;
    }


    // [ Actor ]
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

        PxRigidBodyExt::updateMassAndInertia(*dyn, d.density); // 질량 계산 : Shape 부피 × density
        m_Actor = dyn;
    }

    m_Actor->attachShape(*m_Shape);
    phys.GetScene()->addActor(*m_Actor); // 물리 씬에 추가 

    m_BodyType = body;
    m_ColliderType = collider;
}


// ------------------------------
// 좌표 변환
// ------------------------------

// Transform → Physics  
//                      ** Dynamic에는 매 프레임 쓰면 안 됨
void PhysicsComponent::SyncToPhysics()
{
    if (!m_Actor || !transform) return;

    PxTransform px;
    px.p = ToPx(transform->position);
    px.q = ToPxQuat(XMLoadFloat4(&transform->rotation)); // Quaternion 그대로

    m_Actor->setGlobalPose(px);
}

// Physics → Transform (물리 시뮬 하고나서 매 프레임 실행)
void PhysicsComponent::SyncFromPhysics()
{
    if (!m_Actor || !transform) return;

    PxTransform px = m_Actor->getGlobalPose();

    transform->position = ToDX(px.p);       // 위치 변환
    transform->rotation = ToDXQuatF4(px.q); // Quaternion 그대로
}
