#pragma once
#include <PxPhysicsAPI.h>

using namespace physx;

PxFilterFlags PhysicsFilterShader(
    PxFilterObjectAttributes attr0, PxFilterData data0,
    PxFilterObjectAttributes attr1, PxFilterData data1,
    PxPairFlags& pairFlags,
    const void*, PxU32); 