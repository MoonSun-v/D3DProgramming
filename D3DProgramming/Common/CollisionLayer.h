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
    Ball        = 1 << 6
};

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