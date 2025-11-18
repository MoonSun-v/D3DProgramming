#include "AssetManager.h"
#include <memory>
#include <string>

std::shared_ptr<SkeletalMeshAsset>
AssetManager::LoadSkeletalMesh(ID3D11Device* device, const std::string& filePath)
{
    AssetKey key(filePath, EAssetKind::SkeletalMesh);

    auto it = m_SkeletalMeshMap.find(key);

    if (it != m_SkeletalMeshMap.end())
    {
        if (auto exist = it->second.lock())
            return exist;
        else
            m_SkeletalMeshMap.erase(it);
    }

    auto asset = std::make_shared<SkeletalMeshAsset>();
    asset->LoadFromFBX(device, filePath);

    m_SkeletalMeshMap[key] = asset;
    return asset;
}