#pragma once
#include "../Common/AnimationState.h"
#include "../Common/AnimationController.h"


// [ Dance1의 상태가 되면, 3초 후에 Dance_2로 전환 ]

class Dance1State : public AnimationState
{
public:
    float Time = 0.0f;

    Dance1State(
        const AnimationClip* clip, AnimationController* controller)
        : AnimationState("Dance_1", clip, controller) {
    }

    void OnEnter() override
    {
        Time = 0.0f;
    }

    void OnUpdate(float dt) override
    {
        // OutputDebugString((L"[Dance1State] Update dalta : " + std::to_wstring(dt) + L"\n").c_str());

        // TODO : 업데이트 순서 관리 확인 필요!!
        if (dt > 1.0f) dt = 0.0f;
        Time += dt;

        // 5초 뒤 자동 전환
        if (Time >= 5.0f)
        {
            // 전환 
            Controller->ChangeState("Dance_2", 0.4f);

            // [디버그용] 전환 기록
            //if (std::find(Transitions.begin(), Transitions.end(), "Dance_2") == Transitions.end())
            //{
            //    Transitions.push_back("Dance_2");
            //}
        }
    }
};