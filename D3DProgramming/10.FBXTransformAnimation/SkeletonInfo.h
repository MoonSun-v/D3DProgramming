#pragma once
#include <iostream>
#include <string>
#include <map>
#include <assimp/scene.h>
#include <directxtk/SimpleMath.h>
#include "BoneMatrixContainer.h"

using namespace DirectX::SimpleMath;

struct BoneInfo
{
	Matrix RelativeTransform; // node->mTransformation (converted & transposed)
	std::string Name;
	std::string ParentBoneName;
};

// �� ��ü ������ �����ϴ� ����ü
class SkeletonInfo
{
public:
	// GPU�� ������ �� ������ ��Ʈ���� �����̳�
	BoneMatrixContainer BoneOffsetMatrices; // �� ������ ��Ʈ���� �迭
	std::vector<BoneInfo> Bones;
	std::map<std::string, int> BoneMappingTable; // BoneName, BoneIndex
	std::map<std::string, int> MeshMappingTable; // MeshName, BoneIndex

public:
	void CreateFromAiScene(const aiScene* pScene);
	BoneInfo* CreateBoneInfo(const aiScene* pScene, const aiNode* pNode);

	BoneInfo* GetBoneInfoByName(const std::string& name);
	BoneInfo* GetBoneInfoByIndex(int index);
	int GetBoneIndexByBoneName(const std::string& boneName);
	int GetBoneIndexByMeshName(const std::string& meshName);
	int GetBoneCount() { return (int)Bones.size(); }
	const std::string& GetBoneName(int index) { return Bones[index].Name; }

	void CountNode(int& Count, const aiNode* pNode);
};

