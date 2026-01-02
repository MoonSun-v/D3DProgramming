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

void Animator::Play(const AnimationClip* clip, float blendTime)
{
    if (!clip) return;

    if (!m_Current || blendTime <= 0.0f)
    {
        m_Current = clip;
        m_Time = 0.0f;
        m_Next = nullptr;
        return;
    }

    // 블렌딩 전환
    m_Next = clip;
    m_BlendDuration = blendTime;
    m_BlendTime = 0.0f;
}

void Animator::Update(float deltaTime)
{
    if (!m_Skeleton) return; // Skeleton 없으면 그냥 종료

    // -----------------------------------------
    // 1. 애니메이션 재생 중이면 Pose 계산
    // -----------------------------------------
    if (m_Current)
    {
        m_Time += deltaTime;
        m_Time = fmod(m_Time, m_Current->Duration);

        // 본별 로컬 변환 행렬 계산
        m_Current->EvaluatePose(m_Time, m_Skeleton, m_CurrentPose);

        // [ 블렌딩 처리 ]
        if (m_Next)
        {
            m_BlendTime += deltaTime;
            float t = m_BlendTime / m_BlendDuration;
            t = Clamp(t, 0.0f, 1.0f);

            m_Next->EvaluatePose(m_Time, m_Skeleton, m_NextPose);

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

            OutputDebugStringW(L"[Animator] m_Next 포즈 블랜딩 \n");
        }
        else
        {
            m_FinalPose = m_CurrentPose;
        }
    }

    // -----------------------------------------
    // 2. Play 중이 아니면 T-pose 사용
    // -----------------------------------------
    else
    {
        for (int i = 0; i < m_FinalPose.size(); ++i)
        {
            // SkeletonInfo에 저장된 바인드 포즈를 그대로 사용
            m_FinalPose[i] = m_Skeleton->GetBindPose(i);
        }
    }
}