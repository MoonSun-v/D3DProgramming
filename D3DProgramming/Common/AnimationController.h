#pragma once
#include "Animator.h"
#include <map>

struct AnimationState
{
    std::string Name;
    const AnimationClip* Clip;
};

struct AnimationTransition
{
    std::string From;
    std::string To;
    float BlendTime;

    // 조건 데이터
    // std::function<bool()> Condition;
    float MinStateTime = 0.0f;
};

class AnimationController
{
public:
    std::map<std::string, AnimationState> States;
    std::vector<AnimationTransition> Transitions;

    std::string CurrentState;
    float StateTime = 0.0f;

    void Update(float dt, Animator& animator);
    void ChangeState(const std::string& next, Animator& animator, float blend);
};