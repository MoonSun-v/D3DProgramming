#include "Animation.h"
#include <algorithm>

// BoneAnimation::Evaluate - time(√ ) ±‚¡ÿ
Matrix BoneAnimation::Evaluate(float time) const
{
    if (AnimationKeys.empty())
        return Matrix::Identity;

    // if single key
    if (AnimationKeys.size() == 1)
    {
        const AnimationKey& k = AnimationKeys[0];
        return Matrix::CreateScale(k.Scaling)
            * Matrix::CreateFromQuaternion(k.Rotation)
            * Matrix::CreateTranslation(k.Position);
    }

    // find interval
    size_t i = 0;
    while (i + 1 < AnimationKeys.size() && time >= AnimationKeys[i + 1].Time) ++i;
    if (i + 1 >= AnimationKeys.size()) i = AnimationKeys.size() - 2;

    const AnimationKey& k0 = AnimationKeys[i];
    const AnimationKey& k1 = AnimationKeys[i + 1];

    float span = k1.Time - k0.Time;
    float t = (span > 0.0f) ? ((time - k0.Time) / span) : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    Vector3 pos = Vector3::Lerp(k0.Position, k1.Position, t);
    Vector3 scl = Vector3::Lerp(k0.Scaling, k1.Scaling, t);
    Quaternion rot = Quaternion::Slerp(k0.Rotation, k1.Rotation, t);
    rot.Normalize();

    return Matrix::CreateScale(scl)
        * Matrix::CreateFromQuaternion(rot)
        * Matrix::CreateTranslation(pos);
}

void BoneAnimation::Evaluate(float time, Vector3& outPos, Quaternion& outRot, Vector3& outScale) const
{
    if (AnimationKeys.empty())
    {
        outPos = Vector3::Zero;
        outRot = Quaternion::Identity;
        outScale = Vector3::One;
        return;
    }

    if (AnimationKeys.size() == 1)
    {
        const AnimationKey& k = AnimationKeys[0];
        outPos = k.Position;
        outRot = k.Rotation;
        outScale = k.Scaling;
        return;
    }

    size_t i = 0;
    while (i + 1 < AnimationKeys.size() && time >= AnimationKeys[i + 1].Time) ++i;
    if (i + 1 >= AnimationKeys.size()) i = AnimationKeys.size() - 2;

    const AnimationKey& k0 = AnimationKeys[i];
    const AnimationKey& k1 = AnimationKeys[i + 1];

    float span = k1.Time - k0.Time;
    float t = (span > 0.0f) ? ((time - k0.Time) / span) : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    outPos = Vector3::Lerp(k0.Position, k1.Position, t);
    outScale = Vector3::Lerp(k0.Scaling, k1.Scaling, t);
    outRot = Quaternion::Slerp(k0.Rotation, k1.Rotation, t);
    outRot.Normalize();
}

Matrix Animation::GetBoneTransform(int boneIndex, float time) const
{
    if (boneIndex < 0 || boneIndex >= (int)BoneAnimations.size())
        return Matrix::Identity;
    return BoneAnimations[boneIndex].Evaluate(time);
}