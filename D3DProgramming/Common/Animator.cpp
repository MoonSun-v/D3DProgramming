#include "Animator.h"

template<typename T>
T Clamp(T v, T min, T max)
{
    return (v < min) ? min : (v > max) ? max : v;
}

static TRS DecomposeTRS(const Matrix& m)
{
    TRS t;

    DirectX::XMVECTOR scale;
    DirectX::XMVECTOR rot;
    DirectX::XMVECTOR pos;

    DirectX::XMMatrixDecompose(
        &scale,
        &rot,
        &pos,
        m
    );

    t.scale = Vector3(scale);
    t.rot = Quaternion(rot);
    t.pos = Vector3(pos);
    t.rot.Normalize();

    return t;
}

static Matrix ComposeTRS(const TRS& t)
{
    return
        Matrix::CreateScale(t.scale) *
        Matrix::CreateFromQuaternion(t.rot) *
        Matrix::CreateTranslation(t.pos);
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
    if (!clip) 
        return;

    // 이미 같은 애니메이션이면 무시
    if (m_Current == clip && m_Next == nullptr)
        return;

    if (!m_Current || blendTime <= 0.0f)
    {
        m_Current = clip;
        m_Time = 0.0f;
        m_Next = nullptr;
        return;
    }

    m_Next = clip;
    m_NextTime = 0.0f;
    m_BlendDuration = blendTime;
    m_BlendTime = 0.0f;
}

void Animator::Update(float deltaTime)
{
    if (!m_Skeleton) return; 

    // -----------------------------------------
    // 1. 애니메이션 재생 중이면 Pose 계산
    // -----------------------------------------
    if (m_Current)
    {
        m_Time += deltaTime;
        m_Time = fmod(m_Time, m_Current->Duration);

        m_Current->EvaluatePose(m_Time, m_Skeleton, m_CurrentPose);

        // 블렌딩 처리 
        if (m_Next)
        {
            m_BlendTime += deltaTime;
            float alpha = Clamp(m_BlendTime / m_BlendDuration, 0.0f, 1.0f);

            float t_current = m_Time;   // Current 애니메이션 시간
            float t_next = m_NextTime;  // Next 애니메이션 시간

            m_Current->EvaluatePose(t_current, m_Skeleton, m_CurrentPose);
            m_Next->EvaluatePose(t_next, m_Skeleton, m_NextPose);
            
            // 본 단위 블렌딩
            for (int i = 0; i < m_FinalPose.size(); ++i)
            {
                TRS a = DecomposeTRS(m_CurrentPose[i]);
                TRS b = DecomposeTRS(m_NextPose[i]);

                TRS r;
                r.pos = Vector3::Lerp(a.pos, b.pos, alpha);
                r.rot = Quaternion::Slerp(a.rot, b.rot, alpha);
                r.scale = Vector3::Lerp(a.scale, b.scale, alpha);

                m_FinalPose[i] = ComposeTRS(r);
            }

            // 블렌딩 완료 시점
            if (alpha >= 1.0f)
            {
                m_Current = m_Next;
                m_Next = nullptr;
                m_CurrentPose = m_FinalPose; 
                m_Time = t_next;  // 다음 애니메이션 진행 시간 그대로 유지..! 
            }
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
            // SkeletonInfo에 저장된 바인드 포즈 그대로 사용
            m_FinalPose[i] = m_Skeleton->GetBindPose(i);
        }
    }
}