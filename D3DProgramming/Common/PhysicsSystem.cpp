#include "PhysicsSystem.h"
#include "PhysXUtils.h"
#include <extensions/PxDefaultCpuDispatcher.h>

PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

bool PhysicsSystem::Initialize()
{
    // 1. Foundation
    m_Foundation = PxCreateFoundation(
        PX_PHYSICS_VERSION,
        m_Allocator,
        m_ErrorCallback
    );
    if (!m_Foundation)
        return false;

    // 2. PVD


    // 3. Physics
    m_Physics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *m_Foundation,
        PxTolerancesScale(),
        true
    );
    if (!m_Physics)
        return false;

    // 4. Scene
    PxSceneDesc sceneDesc(m_Physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);

    m_Dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = m_Dispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    m_Scene = m_Physics->createScene(sceneDesc);
    if (!m_Scene)
        return false;

    // 5. PVD Scene 설정 (있어야 충돌/접촉 보임)
    if (PxPvdSceneClient* client = m_Scene->getScenePvdClient())
    {
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
        client->setScenePvdFlag(PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
    }

    // 6. Default Material
    m_DefaultMaterial = m_Physics->createMaterial(
        0.5f, // static friction
        0.5f, // dynamic friction
        0.6f  // restitution
    );

    return true;
}

void PhysicsSystem::Shutdown()
{
    PX_RELEASE(m_Scene);
    PX_RELEASE(m_Dispatcher);
    PX_RELEASE(m_DefaultMaterial);
    PX_RELEASE(m_Physics);
    PX_RELEASE(m_Foundation);
}

void PhysicsSystem::Simulate(float dt)
{
    if (!m_Scene)
        return;

    m_Scene->simulate(dt);
    m_Scene->fetchResults(true);
}
