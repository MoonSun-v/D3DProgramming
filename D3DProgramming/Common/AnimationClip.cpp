#include "AnimationClip.h"
#include <algorithm>


// [ 키 프레임 보간 함수 ] 
template<typename KeyType, typename ValueType>
static ValueType SampleTrack(
    const std::vector<KeyType>& keys,
    float time,
    const ValueType& defaultValue)
{
    // 키가 없을 경우 기본값 반환
    if (keys.empty())
    {
        // OutputDebugStringW(L"[AnimationClip] 키가 없으므로 기본값 반환\n");
        return defaultValue;
    }

    // 키가 하나뿐일 경우 해당 키 값 반환
    if (keys.size() == 1 || time <= keys.front().Time)
    {
        // OutputDebugStringW(L"[AnimationClip] 키가 하나이므로, 해당 키 반환\n");
        return keys.front().Value;
    }

    if (time >= keys.back().Time) return keys.back().Value;

    for (size_t i = 0; i + 1 < keys.size(); ++i)
    {
        const auto& k0 = keys[i];
        const auto& k1 = keys[i + 1];

        if (time >= k0.Time && time <= k1.Time)
        {
            float span = k1.Time - k0.Time;
            float t = (span > 0.0f) ? (time - k0.Time) / span : 0.0f;

            if constexpr (std::is_same_v<ValueType, Quaternion>)
            {
                Quaternion q = Quaternion::Slerp(k0.Value, k1.Value, t);
                q.Normalize();
                return q;
            }
            else
            {
                return ValueType::Lerp(k0.Value, k1.Value, t);
            }
        }
    }
    OutputDebugString((L"[AnimationClip] keys.size() : " + std::to_wstring(keys.size()) + L"\n").c_str());

    return keys.back().Value;
}

// ------------------- BoneAnimation -------------------

void BoneAnimation::Evaluate(
    float time,
    Vector3& outPos,
    Quaternion& outRot,
    Vector3& outScale) const
{
    outPos = SampleTrack<PositionKey, Vector3>(Positions, time, Vector3::Zero);
    outRot = SampleTrack<RotationKey, Quaternion>(Rotations, time, Quaternion::Identity);
    outScale = SampleTrack<ScaleKey, Vector3>(Scales, time, Vector3::One);
}

// ------------------- Animation Clip -------------------

void AnimationClip::EvaluatePose(float time,const SkeletonInfo* skeleton, std::vector<Matrix>& outLocalPose) const
{
    const int boneCount = skeleton->GetBoneCount();
    outLocalPose.resize(boneCount);

    // =========================================================
    // 1. 기본값: Bind Pose (RelativeTransform)
    //    -> 애니메이션이 없는 본은 T-Pose 유지
    // =========================================================
    for (int i = 0; i < boneCount; ++i)
    {
        outLocalPose[i] = skeleton->Bones[i].RelativeTransform;
    }

    // =========================================================
    // 2. 애니메이션이 존재하는 본만 덮어쓰기
    // =========================================================
    const int animBoneCount = (int)BoneAnimations.size();

    for (int i = 0; i < animBoneCount; ++i)
    {
        const BoneAnimation& anim = BoneAnimations[i];

        Vector3 pos = Vector3::Zero;
        Vector3 scale = Vector3::One;
        Quaternion rot = Quaternion::Identity;

        anim.Evaluate(time, pos, rot, scale);

        Matrix animLocal =
            Matrix::CreateScale(scale) *
            Matrix::CreateFromQuaternion(rot) *
            Matrix::CreateTranslation(pos);

        // -----------------------------------------------------
        // FBX / Assimp 애니메이션은 대부분
        // "Bind Pose 기준 Local" 이므로
        //
        // Local = AnimLocal * BindLocal
        // -----------------------------------------------------
        outLocalPose[i] = animLocal * skeleton->Bones[i].RelativeTransform.Transpose();
    }
}