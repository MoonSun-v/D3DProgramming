#pragma once
#include <PxPhysicsAPI.h>
using namespace physx;

class PhysicsSystem
{
public:
    PhysicsSystem() = default;
    ~PhysicsSystem();

    bool Initialize();
    void Shutdown();

    void Simulate(float dt);

    PxPhysics* GetPhysics() const { return m_Physics; }
    PxScene* GetScene()   const { return m_Scene; }
    PxMaterial* GetDefaultMaterial() const { return m_DefaultMaterial; }

private:
    PxDefaultAllocator      m_Allocator;
    PxDefaultErrorCallback  m_ErrorCallback;

    PxFoundation* m_Foundation = nullptr;
    PxPhysics* m_Physics = nullptr;
    PxScene* m_Scene = nullptr;
    PxMaterial* m_DefaultMaterial = nullptr;
    PxCpuDispatcher* m_Dispatcher = nullptr;
};