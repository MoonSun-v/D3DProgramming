#pragma once
#include<iostream>

#include <assimp/scene.h>
#include <directxtk/SimpleMath.h>

using namespace DirectX::SimpleMath;

struct AnimationKey
{
	float Time;          // Ű������ �ð� (��)
	Vector3 Position;    // ��ġ Ű
	Quaternion Rotation; // ȸ�� Ű
	Vector3 Scaling;     // ������ Ű
};

class BoneAnimation
{
public:
	std::vector<AnimationKey> AnimationKeys;

	// Ư�� �ð� t�� �ش��ϴ� ���� ��ȯ��� ���
	Matrix Evaluate(float time) const;
	void Evaluate(float time, Vector3& outPos, Quaternion& outRot, Vector3& outScale) const; 
};

class Animation
{
public:
	std::string Name;
	float Duration = 0.0f;
	std::vector<BoneAnimation> BoneAnimations;  // �� ���� Ű ������

public:
	Matrix GetBoneTransform(int boneIndex, float time) const;
};

