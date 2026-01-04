#pragma once
#include<iostream>

#include "SkeletonInfo.h"
#include <unordered_map>

#include <assimp/scene.h>
#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;


// ------------------- Key Structs -------------------
struct PositionKey
{
    float Time = 0.0f;
    Vector3 Value = Vector3::Zero;
};
struct RotationKey
{
    float Time = 0.0f;
    Quaternion Value = Quaternion::Identity;
};
struct ScaleKey
{
    float Time = 0.0f;
    Vector3 Value = Vector3::One;
};


// ----------------------------------------------------
// [ BoneAnimation ] 
// 
// 특정 본 하나에 대한 애니메이션 트랙으로,
// 위치·회전·스케일 키프레임을 시간에 따라 보간
// ----------------------------------------------------
class BoneAnimation
{
public:
    std::vector<PositionKey> Positions;
    std::vector<RotationKey> Rotations;
    std::vector<ScaleKey>    Scales;

    void Evaluate(
        float time,
        Vector3& outPos,
        Quaternion& outRot,
        Vector3& outScale
    ) const;
};


// ----------------------------------------------------
// [ AnimationClip ] 
// 
// 각 본의 키프레임 정보를 통해, 특정 시간의 포즈 평가
// (하나의 애니메이션 데이터) 
// ----------------------------------------------------
class AnimationClip
{
public:
    std::string Name;
    float Duration = 0.0f; // 애니메이션 총 길이
    bool Loop = true;

    std::vector<BoneAnimation> BoneAnimations; // 각 본별 키 데이터

    void EvaluatePose(
        float time,
        const SkeletonInfo* skeleton,
        std::vector<Matrix>& outLocalPose) const;
};