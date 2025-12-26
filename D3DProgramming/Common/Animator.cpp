#include "Animator.h"

template<typename T>
T Clamp(T v, T min, T max)
{
    return (v < min) ? min : (v > max) ? max : v;
}

void Animator::Initialize(const SkeletonInfo* skeleton)
{
    m_Skeleton = skeleton;
    int count = skeleton->GetBoneCount();
    m_CurrentPose.resize(count);
    m_NextPose.resize(count);
    m_FinalPose.resize(count);
}

void Animator::Play(const Animation* clip, float blendTime)
{
    if (!m_Current || blendTime <= 0.0f)
    {
        m_Current = clip;
        m_Time = 0.0f;
        m_Next = nullptr;
        return;
    }

    m_Next = clip;
    m_BlendDuration = blendTime;
    m_BlendTime = 0.0f;
}

void Animator::Update(float deltaTime)
{
    if (!m_Current) return;

    m_Time += deltaTime;
    m_Time = fmod(m_Time, m_Current->Duration);

    m_Current->EvaluatePose(m_Time, m_CurrentPose);

    if (m_Next)
    {
        m_BlendTime += deltaTime;
        float t = m_BlendTime / m_BlendDuration;
        t = Clamp(t, 0.0f, 1.0f);

        m_Next->EvaluatePose(m_Time, m_NextPose);

        // 본 단위 블렌딩
        for (int i = 0; i < m_FinalPose.size(); ++i)
        {
            m_FinalPose[i] = Matrix::Lerp(m_CurrentPose[i], m_NextPose[i], t);
        }

        if (t >= 1.0f)
        {
            m_Current = m_Next;
            m_Next = nullptr;
        }
    }
    else
    {
        m_FinalPose = m_CurrentPose;
    }
}