#pragma once

enum class QueryTriggerInteraction
{
    UseGlobal, // (지금은 Collide와 동일 취급)
    Ignore,    // Trigger 무시
    Collide    // Trigger 포함
};