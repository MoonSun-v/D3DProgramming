#include "PhysicsFilterShader.h"

PxFilterFlags PhysicsFilterShader(
    PxFilterObjectAttributes attr0, PxFilterData data0,
    PxFilterObjectAttributes attr1, PxFilterData data1,
    PxPairFlags& pairFlags,
    const void*, PxU32)
{
    const bool isTrigger =
        PxFilterObjectIsTrigger(attr0) ||
        PxFilterObjectIsTrigger(attr1);

    // Layer 검사 (Trigger 포함)
    bool collide =
        (data0.word0 & data1.word1) &&
        (data1.word0 & data0.word1);

    if (!collide)
        return PxFilterFlag::eSUPPRESS;

    if (isTrigger)
    {
        pairFlags =
            PxPairFlag::eTRIGGER_DEFAULT |
            PxPairFlag::eNOTIFY_TOUCH_FOUND |
            PxPairFlag::eNOTIFY_TOUCH_LOST;

        return PxFilterFlag::eDEFAULT;
    }

    // 일반 Collision
    pairFlags =
        PxPairFlag::eCONTACT_DEFAULT |
        PxPairFlag::eNOTIFY_TOUCH_FOUND |
        PxPairFlag::eNOTIFY_TOUCH_PERSISTS |
        PxPairFlag::eNOTIFY_TOUCH_LOST;

    return PxFilterFlag::eDEFAULT;
}
