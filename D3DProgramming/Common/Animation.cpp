#include "Animation.h"
#include <algorithm>

// 주어진 시간(time)에 해당하는 본의 변환 행렬 계산
Matrix BoneAnimation::Evaluate(float time) const
{
    // 애니메이션 키가 비어있으면 단위 행렬 반환 (즉, 변환 없음)
    if (AnimationKeys.empty())
        return Matrix::Identity;

    // 애니메이션 키가 하나뿐인 경우 : 그대로 반환
    if (AnimationKeys.size() == 1)
    {
        const AnimationKey& k = AnimationKeys[0];
        return Matrix::CreateScale(k.Scaling)
            * Matrix::CreateFromQuaternion(k.Rotation)
            * Matrix::CreateTranslation(k.Position);
    }

    // 1. 현재 시간(time)에 해당하는 키 구간 찾기
    size_t i = 0;
    while (i + 1 < AnimationKeys.size() && time >= AnimationKeys[i + 1].Time) ++i;

    // time이 마지막 키보다 큰 경우 마지막 두 키를 사용
    if (i + 1 >= AnimationKeys.size()) i = AnimationKeys.size() - 2;

    // 현재 구간의 두 키 (이전 프레임과 다음 프레임)
    const AnimationKey& k0 = AnimationKeys[i];
    const AnimationKey& k1 = AnimationKeys[i + 1];

    // 2. 두 키 사이에서 보간 인자(t) 계산
    float span = k1.Time - k0.Time; // 두 키 간의 시간 차
    float t = (span > 0.0f) ? ((time - k0.Time) / span) : 0.0f; // 보간 비율
    if (t < 0.0f) t = 0.0f; // 0~1 범위로 제한
    if (t > 1.0f) t = 1.0f;

    // 3. 위치 / 스케일 / 회전 보간
    Vector3 pos = Vector3::Lerp(k0.Position, k1.Position, t);       // 선형보간 
    Vector3 scl = Vector3::Lerp(k0.Scaling, k1.Scaling, t);         // 선형보간 
    Quaternion rot = Quaternion::Slerp(k0.Rotation, k1.Rotation, t); // 회전은 구면선형보간(Slerp)
    rot.Normalize();

    // 4. 최종 변환 행렬 (S * R * T)
    return Matrix::CreateScale(scl)
        * Matrix::CreateFromQuaternion(rot)
        * Matrix::CreateTranslation(pos);
}

// Evaluate (오버로드 버전) : 행렬 대신 개별 요소 반환
void BoneAnimation::Evaluate(float time, Vector3& outPos, Quaternion& outRot, Vector3& outScale) const
{
    // 키가 없을 경우 기본값 반환
    if (AnimationKeys.empty())
    {
        outPos = Vector3::Zero;
        outRot = Quaternion::Identity;
        outScale = Vector3::One;
        return;
    }

    // 키가 하나뿐일 경우 해당 키 값 반환
    if (AnimationKeys.size() == 1)
    {
        const AnimationKey& k = AnimationKeys[0];
        outPos = k.Position;
        outRot = k.Rotation;
        outScale = k.Scaling;
        return;
    }

    // 1. 현재 시간에 맞는 키 구간 찾기
    size_t i = 0;
    while (i + 1 < AnimationKeys.size() && time >= AnimationKeys[i + 1].Time) ++i;
    if (i + 1 >= AnimationKeys.size()) i = AnimationKeys.size() - 2;

    const AnimationKey& k0 = AnimationKeys[i];
    const AnimationKey& k1 = AnimationKeys[i + 1];

    // 2. 보간 인자 계산
    float span = k1.Time - k0.Time;
    float t = (span > 0.0f) ? ((time - k0.Time) / span) : 0.0f;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    // 3. 보간 수행
    outPos = Vector3::Lerp(k0.Position, k1.Position, t);
    outScale = Vector3::Lerp(k0.Scaling, k1.Scaling, t);
    outRot = Quaternion::Slerp(k0.Rotation, k1.Rotation, t);
    outRot.Normalize();
}

// 특정 본(bone)의 주어진 시간(time)에 대한 변환 행렬 반환
Matrix Animation::GetBoneTransform(int boneIndex, float time) const
{
    // 유효하지 않은 본 인덱스인 경우 단위 행렬 반환
    if (boneIndex < 0 || boneIndex >= (int)BoneAnimations.size())  return Matrix::Identity;

    // 해당 본 애니메이션
    return BoneAnimations[boneIndex].Evaluate(time);
}