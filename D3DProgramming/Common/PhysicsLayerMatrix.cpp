#include "PhysicsLayerMatrix.h"

inline uint32_t LayerToIndex(CollisionLayer layer)
{
    uint32_t value = (uint32_t)layer;
    uint32_t index = 0;

    // value가 1이 될 때까지 오른쪽으로 시프트
    while (value > 1)
    {
        value >>= 1;
        ++index;
    }

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

    // [1] IgnoreTest <-> Ball : 서로 충돌 안 함
    s_Matrix[LayerToIndex(CollisionLayer::IgnoreTest)] &= ~(uint32_t)CollisionLayer::Ball;
    s_Matrix[LayerToIndex(CollisionLayer::Ball)] &= ~(uint32_t)CollisionLayer::IgnoreTest;

    // [2] IgnoreTest <-> Player : 서로 충돌 안 함
    s_Matrix[LayerToIndex(CollisionLayer::IgnoreTest)] &= ~(uint32_t)CollisionLayer::Player;
    s_Matrix[LayerToIndex(CollisionLayer::Player)] &= ~(uint32_t)CollisionLayer::IgnoreTest;

}

CollisionMask PhysicsLayerMatrix::GetMask(CollisionLayer layer)
{
    return s_Matrix[LayerToIndex(layer)];
}

bool PhysicsLayerMatrix::CanCollide(CollisionLayer a, CollisionLayer b)
{
    return (GetMask(a) & (CollisionMask)b) != 0;
}