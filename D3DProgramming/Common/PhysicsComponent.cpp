#include "PhysicsComponent.h"
#include "PhysicsSystem.h"
#include "PhysXUtils.h"
#include "Transform.h"

PhysicsComponent::~PhysicsComponent()
{
    PX_RELEASE(m_Actor);
}

// ------------------------------
// 寇何 API
// ------------------------------
void PhysicsComponent::CreateStaticBox(const Vector3& half)
{
    ColliderDesc d;
    d.halfExtents = half;
    CreateCollider(ColliderType::Box, PhysicsBodyType::Static, d);
}

void PhysicsComponent::CreateDynamicBox(const Vector3& half, float density)
{
    ColliderDesc d;
    d.halfExtents = half;
    d.density = density;
    CreateCollider(ColliderType::Box, PhysicsBodyType::Dynamic, d);
}

void PhysicsComponent::CreateStaticSphere(float radius)
{
    ColliderDesc d;
    d.radius = radius;
    CreateCollider(ColliderType::Sphere, PhysicsBodyType::Static, d);
}

void PhysicsComponent::CreateDynamicSphere(float radius, float density)
{
    ColliderDesc d;
    d.radius = radius;
    d.density = density;
    CreateCollider(ColliderType::Sphere, PhysicsBodyType::Dynamic, d);
}

void PhysicsComponent::CreateDynamicCapsule(float radius, float height, float density)
{
    ColliderDesc d;
    d.radius = radius;
    d.height = height;
    d.density = density;
    CreateCollider(ColliderType::Capsule, PhysicsBodyType::Dynamic, d);
}

// ------------------------------
// 郴何 积己 肺流
// ------------------------------
void PhysicsComponent::CreateCollider(
    ColliderType collider,
    PhysicsBodyType body,
    const ColliderDesc& d)
{
    auto& phys = PhysicsSystem::Get();
    PxPhysics* px = phys.GetPhysics();
    PxMaterial* mat = phys.GetDefaultMaterial();

    // Shape
    switch (collider)
    {
    case ColliderType::Box:
        m_Shape = px->createShape(
            PxBoxGeometry(d.halfExtents.x, d.halfExtents.y, d.halfExtents.z),
            *mat);
        break;

    case ColliderType::Sphere:
        m_Shape = px->createShape(
            PxSphereGeometry(d.radius),
            *mat);
        break;

    case ColliderType::Capsule:
        m_Shape = px->createShape(
            PxCapsuleGeometry(d.radius, d.height * 0.5f),
            *mat);
        break;
    }

    // Actor
    if (body == PhysicsBodyType::Static)
    {
        m_Actor = px->createRigidStatic(PxTransform(PxIdentity));
    }
    else
    {
        PxRigidDynamic* dyn = px->createRigidDynamic(PxTransform(PxIdentity));

        if (body == PhysicsBodyType::Kinematic)
            dyn->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);

        PxRigidBodyExt::updateMassAndInertia(*dyn, d.density);
        m_Actor = dyn;
    }

    m_Actor->attachShape(*m_Shape);
    phys.GetScene()->addActor(*m_Actor);

    m_BodyType = body;
    m_ColliderType = collider;
}

// ------------------------------
// Sync
// ------------------------------
void PhysicsComponent::SyncToPhysics()
{
    if (!m_Actor || !owner) return;

    XMVECTOR q =
        XMQuaternionRotationRollPitchYaw(
            owner->rotation.x,
            owner->rotation.y,
            owner->rotation.z
        );

    PxTransform px;
    px.p = ToPx(owner->position);
    px.q = ToPxQuat(q);

    m_Actor->setGlobalPose(px);
}

void PhysicsComponent::SyncFromPhysics()
{
    if (!m_Actor || !owner) return;

    PxTransform px = m_Actor->getGlobalPose();
    owner->position = ToDX(px.p);

    XMVECTOR q = ToDXQuat(px.q);
    XMFLOAT4 rot;
    XMStoreFloat4(&rot, q);

    owner->rotation = { rot.x, rot.y, rot.z };
}
