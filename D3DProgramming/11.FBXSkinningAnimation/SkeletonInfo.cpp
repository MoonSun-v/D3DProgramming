#include "SkeletonInfo.h"


// 노드 개수 세기 
void SkeletonInfo::CountNode(int& count, const aiNode* pNode)
{
    if (!pNode)  return;

    count++;

    for (unsigned int i = 0; i < pNode->mNumChildren; ++i)
    {
        CountNode(count, pNode->mChildren[i]);
    }
}


// aiScene 로부터 Skeleton 생성
void SkeletonInfo::CreateFromAiScene(const aiScene* pScene)
{
    if (!pScene || !pScene->mRootNode)
    {
        std::cerr << "[SkeletonInfo] Invalid aiScene!" << std::endl;
        return;
    }

    Bones.clear();
    BoneMappingTable.clear();
    MeshMappingTable.clear();

    // 1️. 루트 노드부터 재귀적으로 BoneInfo 생성
    const aiNode* rootNode = pScene->mRootNode;

    // 루트 노드는 Scene Root이므로 실제 본 노드는 자식부터 시작
    std::vector<const aiNode*> nodeStack;
    for (unsigned int i = 0; i < rootNode->mNumChildren; ++i)
        nodeStack.push_back(rootNode->mChildren[i]);

    while (!nodeStack.empty())
    {
        const aiNode* currentNode = nodeStack.back();
        nodeStack.pop_back();

        BoneInfo bone;
        bone.Name = currentNode->mName.C_Str();
        bone.ParentBoneName = (currentNode->mParent) ? currentNode->mParent->mName.C_Str() : "";

        // aiMatrix4x4 -> SimpleMath::Matrix 변환
        aiMatrix4x4 aiMat = currentNode->mTransformation;
        Matrix transform(
            aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
            aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
            aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
            aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
        );

        bone.RelativeTransform = transform.Transpose(); // Row -> Column 보정

        int index = static_cast<int>(Bones.size());
        Bones.push_back(bone);
        BoneMappingTable[bone.Name] = index;


        // 부모 인덱스 연결
        if (!bone.ParentBoneName.empty())
        {
            int parentIndex = GetBoneIndexByBoneName(bone.ParentBoneName);
            Bones[index].ParentIndex = parentIndex;
        }
        else
        {
            Bones[index].ParentIndex = -1; // 루트 본
        }

        // 자식 노드 추가
        for (unsigned int i = 0; i < currentNode->mNumChildren; ++i)
        {
            nodeStack.push_back(currentNode->mChildren[i]);
        }
    }

    // 2️. 메시 정보에서 Bone 오프셋 행렬 추출
    BoneOffsetMatrices.Clear();
    BoneOffsetMatrices.SetBoneCount((int)Bones.size());

    for (unsigned int meshIdx = 0; meshIdx < pScene->mNumMeshes; ++meshIdx)
    {
        const aiMesh* mesh = pScene->mMeshes[meshIdx];
        if (!mesh)
            continue;

        for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
        {
            const aiBone* aiBone = mesh->mBones[boneIdx];
            std::string boneName = aiBone->mName.C_Str();

            int boneIndex = GetBoneIndexByBoneName(boneName);
            if (boneIndex == -1)
            {
                // 없는 본이라면 새로 추가
                BoneInfo extra;
                extra.Name = boneName;
                extra.ParentBoneName = "";
                extra.RelativeTransform = Matrix::Identity;

                boneIndex = (int)Bones.size();
                Bones.push_back(extra);
                BoneMappingTable[boneName] = boneIndex;

                // 오프셋 행렬 초기화
                if (boneIndex < BoneMatrixContainer::MaxBones)
                    BoneOffsetMatrices.SetMatrix(boneIndex, Matrix::Identity);
            }

            // Offset 행렬 저장
            aiMatrix4x4 offset = aiBone->mOffsetMatrix;
            Matrix offsetMat(
                offset.a1, offset.b1, offset.c1, offset.d1,
                offset.a2, offset.b2, offset.c2, offset.d2,
                offset.a3, offset.b3, offset.c3, offset.d3,
                offset.a4, offset.b4, offset.c4, offset.d4
            );
            if (boneIndex < BoneMatrixContainer::MaxBones)
                BoneOffsetMatrices.SetMatrix(boneIndex, offsetMat.Transpose());
        }

        // 메시 이름 -> 첫 번째 본 인덱스 매핑
        std::string meshName = mesh->mName.C_Str();
        if (!meshName.empty())
        {
            if (mesh->mNumBones > 0)
            {
                const char* firstBoneName = mesh->mBones[0]->mName.C_Str();
                int firstBoneIndex = GetBoneIndexByBoneName(firstBoneName);
                MeshMappingTable[meshName] = firstBoneIndex;
            }
        }
    }

    std::cout << "[SkeletonInfo] Bone Count: " << Bones.size() << std::endl;
}


// 이름으로 BoneInfo 찾기
BoneInfo* SkeletonInfo::GetBoneInfoByName(const std::string& name)
{
    int index = GetBoneIndexByBoneName(name);
    if (index == -1)
        return nullptr;
    return &Bones[index];
}


// 인덱스로 BoneInfo 찾기
BoneInfo* SkeletonInfo::GetBoneInfoByIndex(int index)
{
    if (index < 0 || index >= (int)Bones.size())
        return nullptr;
    return &Bones[index];
}


// Bone 이름으로 인덱스 찾기
int SkeletonInfo::GetBoneIndexByBoneName(const std::string& boneName)
{
    auto it = BoneMappingTable.find(boneName);
    if (it != BoneMappingTable.end())
        return it->second;
    return -1;
}


// Mesh 이름으로 인덱스 찾기
int SkeletonInfo::GetBoneIndexByMeshName(const std::string& meshName)
{
    auto it = MeshMappingTable.find(meshName);
    if (it != MeshMappingTable.end())
        return it->second;
    return -1;
}
