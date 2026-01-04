#include "AnimationController.h"

void AnimationController::AddState(std::unique_ptr<AnimationState> state)
{
    States[state->Name] = std::move(state);
}

void AnimationController::Initialize(const SkeletonInfo* skeleton)
{
    AnimatorInstance.Initialize(skeleton);
}

void AnimationController::Update(float dt)
{
    if (CurrentState)
    {
        // OutputDebugString((L"[AnimationController] Update dalta : " + std::to_wstring(dt) + L"\n").c_str());
        CurrentState->OnUpdate(dt);
    }

    AnimatorInstance.Update(dt);
}

void AnimationController::ChangeState(const std::string& name, float blendTime)
{
    if (CurrentState && CurrentState->Name == name)
        return;

    if (CurrentState)
        CurrentState->OnExit();

    auto it = States.find(name);
    if (it == States.end())
        return;

    CurrentState = it->second.get();
    CurrentState->OnEnter();

    AnimatorInstance.Play(CurrentState->Clip, blendTime);
}