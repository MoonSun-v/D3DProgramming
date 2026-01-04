#pragma once
#include <string>

class AnimationController;
class AnimationClip;


// ----------------------------------------------------
// [ AnimationState ] 
// 
// 하나의 애니메이션 상태를 표현
// 상태 진입-업데이트-종료 시점을 통해 전환 조건, 로직 정의
// ----------------------------------------------------

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