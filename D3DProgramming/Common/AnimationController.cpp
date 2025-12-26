#include "AnimationController.h"

void AnimationController::Update(Animator& animator)
{
    for (auto& t : Transitions)
    {
        if (t.From == CurrentState && t.Condition())
        {
            animator.Play(States[t.To].Clip, t.BlendTime);
            CurrentState = t.To;
            break;
        }
    }
}