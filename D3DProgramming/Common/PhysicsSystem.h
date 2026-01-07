#pragma once
#include <PxPhysicsAPI.h>
#include <task/PxCpuDispatcher.h>

using namespace physx;


class PhysicsSystem
{
public:
    static PhysicsSystem& Get() // ½Ì±ÛÅæ 
    {
        static PhysicsSystem instance;
        return instance;
    }

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
    // Core
    PxDefaultAllocator      m_Allocator;
    PxDefaultErrorCallback  m_ErrorCallback;

    PxFoundation* m_Foundation = nullptr;
    PxPhysics* m_Physics = nullptr;
    PxScene* m_Scene = nullptr;
    PxMaterial* m_DefaultMaterial = nullptr;
    PxDefaultCpuDispatcher* m_Dispatcher = nullptr;

    // PVD (µð¹ö±ë¿ë)
    PxPvd* m_Pvd = nullptr;
    PxPvdTransport* m_PvdTransport = nullptr;
};