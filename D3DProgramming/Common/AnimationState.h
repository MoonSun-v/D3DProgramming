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

    // [ 디버그용 ] : 전환 가능한 상태들
    std::vector<std::string> Transitions;

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