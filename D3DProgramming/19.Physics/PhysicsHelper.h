#pragma once
#include <PxPhysicsAPI.h>
#include "../Common/PhysicsSystem.h"

using namespace physx;

class PhysicsHelper
{
public:
    static PxRigidStatic* CreateStaticBox(
        const PxVec3& pos,
        const PxVec3& halfExtents)
    {
        auto& physics = *PhysicsSystem::Get().GetPhysics();
        auto& scene = *PhysicsSystem::Get().GetScene();
        PxMaterial* mat = PhysicsSystem::Get().GetDefaultMaterial();

        PxTransform t(pos);
        PxBoxGeometry geo(halfExtents);

        PxRigidStatic* actor = PxCreateStatic(physics, t, geo, *mat);
        scene.addActor(*actor);
        return actor;
    }

    static PxRigidDynamic* CreateDynamicBox(
        const PxVec3& pos,
        const PxVec3& halfExtents,
        float density)
    {
        auto& physics = *PhysicsSystem::Get().GetPhysics();
        auto& scene = *PhysicsSystem::Get().GetScene();
        PxMaterial* mat = PhysicsSystem::Get().GetDefaultMaterial();

        PxTransform t(pos);
        PxBoxGeometry geo(halfExtents);

        PxRigidDynamic* actor =
            PxCreateDynamic(physics, t, geo, *mat, density);

        scene.addActor(*actor);
        return actor;
    }
};