#pragma once
#include "../Common/AnimationState.h"
#include "../Common/AnimationController.h"

class Dance2State : public AnimationState
{
public:
    float Time = 0.0f;

    Dance2State(
        const AnimationClip* clip, AnimationController* controller)
        : AnimationState("Dance_2", clip, controller) {
    }

    void OnEnter() override
    {
        Time = 0.0f;
    }

    void OnUpdate(float dt) override
    {
        if (dt > 1.0f) dt = 0.0f;
        Time += dt;

        // 애니메이션이 끝났으면 Dance1으로
        if (Clip && Time >= Clip->Duration)
        {
            Controller->ChangeState("Dance_1", 0.4f);

            // [디버그용]
            //if (std::find(Transitions.begin(), Transitions.end(), "Dance_1") == Transitions.end())
            //{
            //    Transitions.push_back("Dance_1");
            //}
        }
    }
};
