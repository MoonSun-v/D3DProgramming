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

// ------------------- BoneAnimation -------------------
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

// ------------------- Animation Clip -------------------
class AnimationClip
{
public:
    std::string Name;
    float Duration = 0.0f;

    std::vector<BoneAnimation> BoneAnimations; // 각 본별 키 데이터

    // 애니메이션이 존재하는 본만 저장
    // std::unordered_map<int, BoneAnimation> BoneAnimations;

    void EvaluatePose(
        float time,
        const SkeletonInfo* skeleton,
        std::vector<Matrix>& outLocalPose) const;
};