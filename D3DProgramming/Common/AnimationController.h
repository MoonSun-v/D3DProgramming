#pragma once
#include "Animator.h"
#include <map>

struct AnimationState
{
    std::string Name;
    const Animation* Clip;
};

struct AnimationTransition
{
    std::string From;
    std::string To;
    float BlendTime;
    std::function<bool()> Condition;
};

class AnimationController
{
public:
    std::map<std::string, AnimationState> States;
    std::vector<AnimationTransition> Transitions;
    std::string CurrentState;

    void Update(Animator& animator);
};

