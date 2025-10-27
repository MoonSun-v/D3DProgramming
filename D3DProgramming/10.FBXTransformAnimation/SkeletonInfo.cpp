#include "SkeletonInfo.h"


// ��� ���� ���� 
void SkeletonInfo::CountNode(int& count, const aiNode* pNode)
{
    if (!pNode)  return;

    count++;

    for (unsigned int i = 0; i < pNode->mNumChildren; ++i)
    {
        CountNode(count, pNode->mChildren[i]);
    }
}


// aiScene �κ��� Skeleton ����
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

    // 1?. ��Ʈ ������ ��������� BoneInfo ����
    const aiNode* rootNode = pScene->mRootNode;

    // ��� ��ȸ �Լ� ��ü (���� ����)
    std::vector<const aiNode*> nodeStack;
    nodeStack.push_back(rootNode);

    while (!nodeStack.empty())
    {
        const aiNode* currentNode = nodeStack.back();
        nodeStack.pop_back();

        BoneInfo bone;
        bone.Name = currentNode->mName.C_Str();
        bone.ParentBoneName = (currentNode->mParent) ? currentNode->mParent->mName.C_Str() : "";

        // aiMatrix4x4 -> SimpleMath::Matrix ��ȯ
        aiMatrix4x4 aiMat = currentNode->mTransformation;
        Matrix transform(
            aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
            aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
            aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
            aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
        );

        bone.RelativeTransform = transform.Transpose(); // Row -> Column ����

        int index = static_cast<int>(Bones.size());
        Bones.push_back(bone);
        BoneMappingTable[bone.Name] = index;

        // �ڽ� ��� �߰�
        for (unsigned int i = 0; i < currentNode->mNumChildren; ++i)
        {
            nodeStack.push_back(currentNode->mChildren[i]);
        }
    }

    // 2?. �޽� �������� Bone ������ ��� ����
    BoneOffsetMatrices.clear();
    BoneOffsetMatrices.resize(Bones.size(), Matrix::Identity);

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
                // ���� ���̶�� ���� �߰�
                BoneInfo extra;
                extra.Name = boneName;
                extra.ParentBoneName = "";
                extra.RelativeTransform = Matrix::Identity;

                boneIndex = (int)Bones.size();
                Bones.push_back(extra);
                BoneMappingTable[boneName] = boneIndex;
                BoneOffsetMatrices.push_back(Matrix::Identity);
            }

            // Offset ��� ����
            aiMatrix4x4 offset = aiBone->mOffsetMatrix;
            Matrix offsetMat(
                offset.a1, offset.b1, offset.c1, offset.d1,
                offset.a2, offset.b2, offset.c2, offset.d2,
                offset.a3, offset.b3, offset.c3, offset.d3,
                offset.a4, offset.b4, offset.c4, offset.d4
            );
            BoneOffsetMatrices[boneIndex] = offsetMat.Transpose();
        }

        // �޽� �̸� -> ù ��° �� �ε��� ����
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


// BoneInfo ���� (ȣ����, ���� ��� X)
BoneInfo* SkeletonInfo::CreateBoneInfo(const aiScene* pScene, const aiScene* pNode)
{
    // ��Ÿ ���� : pNode�� aiScene�� �ƴ϶� aiNode�� �Ǿ�� ������
    // ��� �ñ״�ó�� �״�� ����.
    if (!pNode)  return nullptr;

    BoneInfo bone;
    bone.Name = pNode->mName.C_Str();
    bone.ParentBoneName = (pNode->mParent) ? pNode->mParent->mName.C_Str() : "";

    aiMatrix4x4 aiMat = pNode->mTransformation;
    Matrix transform(
        aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
        aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
        aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
        aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
    );
    bone.RelativeTransform = transform.Transpose();

    Bones.push_back(bone);
    int index = (int)Bones.size() - 1;
    BoneMappingTable[bone.Name] = index;

    return &Bones[index];
}


// �̸����� BoneInfo ã��
BoneInfo* SkeletonInfo::GetBoneInfoByName(const std::string& name)
{
    int index = GetBoneIndexByBoneName(name);
    if (index == -1)
        return nullptr;
    return &Bones[index];
}


// �ε����� BoneInfo ã��
BoneInfo* SkeletonInfo::GetBoneInfoByIndex(int index)
{
    if (index < 0 || index >= (int)Bones.size())
        return nullptr;
    return &Bones[index];
}


// Bone �̸����� �ε��� ã��
int SkeletonInfo::GetBoneIndexByBoneName(const std::string& boneName)
{
    auto it = BoneMappingTable.find(boneName);
    if (it != BoneMappingTable.end())
        return it->second;
    return -1;
}


// Mesh �̸����� �ε��� ã��
int SkeletonInfo::GetBoneIndexByMeshName(const std::string& meshName)
{
    auto it = MeshMappingTable.find(meshName);
    if (it != MeshMappingTable.end())
        return it->second;
    return -1;
}
