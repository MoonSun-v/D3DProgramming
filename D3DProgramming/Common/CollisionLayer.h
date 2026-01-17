#pragma once
#include <cstdint>

enum class CollisionLayer : uint32_t
{
    Default     = 1 << 0,
    Player      = 1 << 1,
    Enemy       = 1 << 2,
    World       = 1 << 3,
    Trigger     = 1 << 4,
    Projectile  = 1 << 5,
    Ball        = 1 << 6,
    IgnoreTest = 1 << 7 // 테스트 하려고 임시로 만든거, 추후 삭제 ㄱㄱ
};
constexpr uint32_t LayerCount = 32;

using CollisionMask = uint32_t;

// --------------------------------------------------
// Layer OR 연산자
// --------------------------------------------------
inline CollisionMask operator|(CollisionLayer a, CollisionLayer b)
{
    return (CollisionMask)a | (CollisionMask)b;
}

// 여러 개 이어서 쓸 수 있게
inline CollisionMask operator|(CollisionMask a, CollisionLayer b)
{
    return a | (CollisionMask)b;
}
