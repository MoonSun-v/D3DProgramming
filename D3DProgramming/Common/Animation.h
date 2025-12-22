#pragma once
#include<iostream>

#include <assimp/scene.h>
#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;

struct AnimationKey
{
	float Time;          // 키프레임 시간 (초)
	Vector3 Position;    // 위치 키
	Quaternion Rotation; // 회전 키
	Vector3 Scaling;     // 스케일 키
};

class BoneAnimation
{
public:
	std::vector<AnimationKey> AnimationKeys;

	// 특정 시간 t에 해당하는 본의 변환행렬 계산
	Matrix Evaluate(float time) const;
	void Evaluate(float time, Vector3& outPos, Quaternion& outRot, Vector3& outScale) const; 
};

class Animation
{
public:
	std::string Name;
	float Duration = 0.0f;
	std::vector<BoneAnimation> BoneAnimations;  // 각 본별 키 데이터

public:
	Matrix GetBoneTransform(int boneIndex, float time) const;
};

