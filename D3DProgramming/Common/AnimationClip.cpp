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
        return defaultValue;

    // 키가 하나뿐일 경우 해당 키 값 반환
    if (keys.size() == 1 || time <= keys.front().Time)
        return keys.front().Value;

    if (time >= keys.back().Time)
        return keys.back().Value;

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

void AnimationClip::EvaluatePose(float time, std::vector<Matrix>& outLocalPose) const
{
    for (size_t i = 0; i < BoneAnimations.size(); ++i)
    {
        Vector3 pos = Vector3::Zero;
        Vector3 scale = Vector3::One;
        Quaternion rot = Quaternion::Identity;

        BoneAnimations[i].Evaluate(time, pos, rot, scale);

        outLocalPose[i] =
            Matrix::CreateScale(scale) *
            Matrix::CreateFromQuaternion(rot) *
            Matrix::CreateTranslation(pos);
    }
}