#include "PhysicsLayerMatrix.h"

inline uint32_t LayerToIndex(CollisionLayer layer)
{
    uint32_t value = (uint32_t)layer;
    uint32_t index = 0;

    while ((value >> index) > 1)
        ++index;

    return index;
}

CollisionMask PhysicsLayerMatrix::s_Matrix[LayerCount];

void PhysicsLayerMatrix::Initialize()
{
    // 기본값 : 모든 레이어는 서로 충돌
    for (int i = 0; i < LayerCount; ++i)
    {
        s_Matrix[i] = 0xFFFFFFFF;
    }

    // -----------------------------
    // 여기서 [ 레이어 체크박스 ] 설정
    // -----------------------------

    // [1] IgnoreTest <-> Ball : IgnoreTest와 Ball은 서로 충돌 안 함 
    uint32_t testIdx = LayerToIndex(CollisionLayer::IgnoreTest);
    uint32_t ballIdx = LayerToIndex(CollisionLayer::Ball);
    s_Matrix[testIdx] &= ~(CollisionMask)CollisionLayer::Ball;
    s_Matrix[ballIdx] &= ~(CollisionMask)CollisionLayer::IgnoreTest;

    // [2] IgnoreTest <-> Player : IgnoreTest와 Player은 서로 충돌 안 함 
    uint32_t playerIdx = LayerToIndex(CollisionLayer::Player);
    s_Matrix[testIdx] &= ~(CollisionMask)CollisionLayer::Player;
    s_Matrix[playerIdx] &= ~(CollisionMask)CollisionLayer::IgnoreTest;

}

CollisionMask PhysicsLayerMatrix::GetMask(CollisionLayer layer)
{
    return s_Matrix[LayerToIndex(layer)] /*| (CollisionMask)CollisionLayer::Trigger*/;
}

bool PhysicsLayerMatrix::CanCollide(CollisionLayer a, CollisionLayer b)
{
    return (GetMask(a) & (CollisionMask)b) != 0;
}