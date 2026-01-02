#include "AnimationController.h"

void AnimationController::Update(float dt, Animator& animator)
{
    // TODO : AnimationController 단일 객체로 만들기?
    if (dt > 1.0f) dt = 0.0f;

    StateTime += dt;
    
    for (auto& t : Transitions)
    {
        if (t.From == CurrentState && StateTime >= t.MinStateTime)
        {
            ChangeState(t.To, animator, t.BlendTime);
            break;
        }
    }

    /*for (auto& t : Transitions)
    {
        if (t.Condition && !t.Condition())
            continue;

        auto nextClip = States[t.To].Clip;
        if (nextClip)
        {
            animator.Play(nextClip, t.BlendTime);
            CurrentState = t.To;
            StateTime = 0.0f;
            break;
        }
    }*/


    //for (auto& t : Transitions)
    //{
    //    if (t.From != CurrentState)
    //        continue;

    //    if (StateTime < t.MinStateTime)
    //        continue;

    //    if (t.Condition && !t.Condition())
    //        continue;

    //    ChangeState(t.To, animator, t.BlendTime);
    //    break;
    //}
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