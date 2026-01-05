#pragma once
#include "../Common/AnimationState.h"
#include "../Common/AnimationController.h"

class IdleState : public AnimationState
{
public:

    IdleState(
        const AnimationClip* clip, AnimationController* controller)
        : AnimationState("Idle_State", clip, controller) {
    }

    void OnEnter() override
    {
        // Idle은 루프 애니메이션
        Controller->AnimatorInstance.Play(Clip, 0.25f);
    }

    void OnUpdate(float dt) override
    {
        auto& params = Controller->Params;

        if (params.GetBool("Die"))
        {
            Controller->ChangeState("Die_State", 0.15f);
            return;
        }

        if (params.GetBool("Attack"))
        {
            Controller->ChangeState("Attack_State", 0.2f);
            return;
        }
    }
};

class AttackState : public AnimationState
{
public:

    AttackState(
        const AnimationClip* clip, AnimationController* controller)
        : AnimationState("Attack_State", clip, controller) {
    }

    void OnEnter() override
    {
        // Attack은 루프 X
        Controller->AnimatorInstance.Play(Clip, 0.15f);
    }

    void OnUpdate(float dt) override
    {
        auto& params = Controller->Params;

        // 죽음은 항상 최우선
        if (params.GetBool("Die"))
        {
            Controller->ChangeState("Die_State", 0.1f);
            return;
        }

        // 공격 애니메이션 끝났는지 확인
        Animator& animator = Controller->AnimatorInstance;

        if (animator.IsCurrentAnimationFinished())
        {
            // FSM 파라미터 정리
            params.SetBool("Attack", false);
            params.SetBool("Idle", true);

            Controller->ChangeState("Idle_State", 0.2f);
        }
    }
};

class DieState : public AnimationState
{
public:

    DieState(
        const AnimationClip* clip, AnimationController* controller)
        : AnimationState("Die_State", clip, controller) {
    }

    void OnEnter() override
    {
        Controller->AnimatorInstance.Play(Clip, 0.1f);
    }

    void OnUpdate(float dt) override
    {
        auto& params = Controller->Params;

        if (params.GetBool("Idle"))
        {
            params.SetBool("Die", false);
            Controller->ChangeState("Idle_State", 0.4f);
        }
        else if (params.GetBool("Attack"))
        {
            params.SetBool("Die", false);
            Controller->ChangeState("Attack_State", 0.4f);
        }
    }
};