#pragma once
#include "Animator.h"
#include "AnimatorParameter.h"
#include "AnimationState.h"
#include <map>


// ----------------------------------------------------
// [ AnimationController ] 
// 
// 애니메이션 FSM의 중심 컨트롤러
// 현재 상태를 관리하고 상태 전환에 따라 Animator에 애니메이션 재생
// ----------------------------------------------------

class AnimationController
{
public:
    Animator AnimatorInstance; // 실제 애니메이션 재생기
    AnimatorParameter Params;

private:
    std::unordered_map<std::string, std::unique_ptr<AnimationState>> States;
    AnimationState* CurrentState = nullptr;

public:
    void Initialize(const SkeletonInfo* skeleton);
    void AddState(std::unique_ptr<AnimationState> state);
    void ChangeState(const std::string& name, float blendTime = 0.2f);
    void Update(float dt);

    AnimationState* GetCurrentState() const { return CurrentState; }

    // [ 디버그용] 
    

public: // [ 디버그용 ]

    // States 읽기용 getter
    const std::unordered_map<std::string, std::unique_ptr<AnimationState>>& GetStates() const
    {
        return States;
    }
    
    // 등록된 상태 이름 리스트 가져오기
    std::vector<std::string> GetStateNames() const
    {
        std::vector<std::string> names;
        for (auto& pair : States)
            names.push_back(pair.first);
        return names;
    }

    // 특정 상태 객체 가져오기
    AnimationState* GetState(const std::string& name) const
    {
        auto it = States.find(name);
        return it != States.end() ? it->second.get() : nullptr;
    }

};