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
    float BlendTime = 0.2f;
    float MinStateTime = 0.0f;
    std::function<bool()> Condition;
    
    AnimationTransition(
        const std::string& from,
        const std::string& to,
        float blend,
        float minTime)
        : From(from), To(to),
        BlendTime(blend),
        MinStateTime(minTime)
    {
    }
};

class AnimationController
{
public:
    AnimationController() : StateTime(0.0f) {}

public:
    std::map<std::string, AnimationState> States;
    std::vector<AnimationTransition> Transitions;

    std::string CurrentState;
    float StateTime = 0.0f;

    void Update(float dt, Animator& animator);
    void ChangeState(const std::string& next, Animator& animator, float blend);
};