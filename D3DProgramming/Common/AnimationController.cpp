#include "AnimationController.h"

void AnimationController::Update(float dt, Animator& animator)
{
    StateTime += dt;

    for (auto& t : Transitions)
    {
        if (t.From == CurrentState && StateTime >= t.MinStateTime)
        {
            ChangeState(t.To, animator, t.BlendTime);
            break;
        }
    }
}

void AnimationController::ChangeState(
    const std::string& next,
    Animator& animator,
    float blend)
{
    if (CurrentState == next) return;

    CurrentState = next;
    StateTime = 0.0f;

    auto it = States.find(next);
    if (it != States.end())
    {
        animator.Play(it->second.Clip, blend);
    }
}