#pragma once
#include "Animator.h"
#include "AnimatorParameter.h"
#include "AnimationState.h"
#include <map>

class AnimationController
{
public:
    Animator AnimatorInstance;
    AnimatorParameters Params;

private:
    std::unordered_map<std::string, std::unique_ptr<AnimationState>> States;
    AnimationState* CurrentState = nullptr;

public:
    void Initialize(const SkeletonInfo* skeleton);
    void AddState(std::unique_ptr<AnimationState> state);
    void ChangeState(const std::string& name, float blendTime = 0.2f);
    void Update(float dt);
};