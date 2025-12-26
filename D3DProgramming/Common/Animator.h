#pragma once
#include "Animation.h"
#include "SkeletonInfo.h"
#include <vector>

class Animator
{
public:
    const SkeletonInfo* m_Skeleton = nullptr;

    const Animation* m_Current = nullptr;
    const Animation* m_Next = nullptr;

    float m_Time = 0.0f;
    float m_BlendTime = 0.0f;
    float m_BlendDuration = 0.0f;

    std::vector<Matrix> m_CurrentPose;
    std::vector<Matrix> m_NextPose;
    std::vector<Matrix> m_FinalPose;

public:
    void Initialize(const SkeletonInfo* skeleton);

    void Play(const Animation* clip, float blendTime=0.0f);

    void Update(float deltaTime);

    const std::vector<Matrix>& GetFinalPose() const { return m_FinalPose; }
};
