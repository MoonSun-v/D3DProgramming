#include "PhysicsSystem.h"

PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

bool PhysicsSystem::Initialize()
{
    m_Foundation = PxCreateFoundation(
        PX_PHYSICS_VERSION,
        m_Allocator,
        m_ErrorCallback
    );
    if (!m_Foundation) return false;

    m_Physics = PxCreatePhysics(
        PX_PHYSICS_VERSION,
        *m_Foundation,
        PxTolerancesScale(),
        true
    );
    if (!m_Physics) return false;

    PxSceneDesc sceneDesc(m_Physics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.f, -9.81f, 0.f);
    m_Dispatcher = PxDefaultCpuDispatcherCreate(2);
    sceneDesc.cpuDispatcher = m_Dispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    m_Scene = m_Physics->createScene(sceneDesc);
    if (!m_Scene) return false;

    m_DefaultMaterial = m_Physics->createMaterial(0.5f, 0.5f, 0.6f);

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
    if (!m_Scene) return;

    m_Scene->simulate(dt);
    m_Scene->fetchResults(true);
}
