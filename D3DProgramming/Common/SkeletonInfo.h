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
	std::string Name;			// 본 이름 
	std::string ParentBoneName;	// 부모 본 이름 
	Matrix RelativeTransform;	// 부모로부터의 상대적인 변환: 원본 FBX 노드 기준 Bind (**Bind pose)

	BoneInfo() = default;
	BoneInfo(const aiNode* pNode)
	{
		Name = std::string(pNode->mName.C_Str());
		RelativeTransform = Matrix(&pNode->mTransformation.a1).Transpose();
	}
};

// BoneInfo를 모은 SkeletonInfo : 본 전체 정보를 저장하는 구조체 
class SkeletonInfo
{
public:
	// GPU에 전달할 본 오프셋 매트릭스 컨테이너
	BoneMatrixContainer BoneOffsetMatrices;			// 본 오프셋 매트릭스 배열
	std::vector<BoneInfo> Bones;
	std::map<std::string, int> BoneMappingTable;	// BoneName, BoneIndex
	std::map<std::string, int> MeshMappingTable;	// MeshName, BoneIndex

public:
	void CreateFromAiScene(const aiScene* pScene);
	// BoneInfo* CreateBoneInfo(const aiScene* pScene, const aiNode* pNode);
	BoneInfo* GetBoneInfoByName(const std::string& name);
	BoneInfo* GetBoneInfoByIndex(int index);
	int GetBoneIndexByBoneName(const std::string& boneName);
	int GetBoneIndexByMeshName(const std::string& meshName);
	int GetBoneCount() const { return (int)Bones.size(); }
	const std::string& GetBoneName(int index) { return Bones[index].Name; }

	void CountNode(int& Count, const aiNode* pNode);
};

