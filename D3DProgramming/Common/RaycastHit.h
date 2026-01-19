#pragma once
#include <PxPhysicsAPI.h>
#include <string>
#include "PhysicsComponent.h"

class RaycastHit
{
public:
    PhysicsComponent* component = nullptr; // 맞은 대상
    physx::PxVec3 point = physx::PxVec3(0);
    physx::PxVec3 normal = physx::PxVec3(0);
    float distance = 0.0f;

    std::string GetName() const
    {
        if (component && component->owner)
            return component->owner->GetName();
        return "Unknown";
    }

    // 디버그 / 확장용
    physx::PxShape* shape = nullptr;
    physx::PxRigidActor* actor = nullptr;
};