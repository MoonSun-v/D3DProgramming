#pragma once
#include "AnimationClip.h"
#include "SkeletonInfo.h"
#include <vector>


// ----------------------------------------------------
// [ Animator ] 
// 
// Animation Clip의 시간 진행, 포즈 평가, 블렌딩 담당
// FSM으로부터 전달받은 애니메이션을 실제 본 포즈로 계산.
// (계산 담당)
// ----------------------------------------------------

struct TRS
{
    Vector3 pos;
    Quaternion rot;
    Vector3 scale;
};

class Animator
{
public:
    const SkeletonInfo* m_Skeleton = nullptr;

    const AnimationClip* m_Current = nullptr;
    const AnimationClip* m_Next = nullptr;

    float m_Time = 0.0f;        // 현재 애니메이션 재생 시간
    float m_NextTime = 0.0f;
    float m_BlendTime = 0.0f;
    float m_BlendDuration = 0.0f;

    std::vector<Matrix> m_CurrentPose;
    std::vector<Matrix> m_NextPose;
    std::vector<Matrix> m_FinalPose;

public:
    void Initialize(const SkeletonInfo* skeleton);
    void Play(const AnimationClip* clip, float blendTime = 0.2f);
    void Update(float deltaTime);

    const std::vector<Matrix>& GetFinalPose() const { return m_FinalPose; }

public:
    // 현재 재생 중인 애니메이션
    const AnimationClip* GetCurrentClip() const { return m_Current; }

    // 현재 애니메이션 재생 시간
    float GetCurrentTime() const { return m_Time; }

    // 다음 애니메이션 (블렌딩 중일 경우)
    const AnimationClip* GetNextClip() const { return m_Next; }

    float GetBlendAlpha() const;
};
