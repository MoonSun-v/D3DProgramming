#pragma once
#include <string>

class AnimationController;
class AnimationClip;

class AnimationState
{
public:
    std::string Name;
    const AnimationClip* Clip = nullptr;
    AnimationController* Controller = nullptr;

    AnimationState(
        const std::string& name,
        const AnimationClip* clip,
        AnimationController* controller)
        : Name(name), Clip(clip), Controller(controller) {
    }

    virtual ~AnimationState() = default;

    virtual void OnEnter() {}
    virtual void OnUpdate(float dt) {}
    virtual void OnExit() {}
};