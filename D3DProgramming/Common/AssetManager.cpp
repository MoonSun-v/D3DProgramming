#include "AssetManager.h"
#include <memory>
#include <string>


std::shared_ptr<SkeletalMeshAsset> AssetManager::LoadSkeletalMesh(ID3D11Device* device, const std::string& filePath)
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

void AssetManager::UnloadSkeletalMesh(const std::string& filePath)
{
    AssetKey key(filePath, EAssetKind::SkeletalMesh);
    auto it = m_SkeletalMeshMap.find(key);
    if (it != m_SkeletalMeshMap.end())
    {
        if (auto asset = it->second.lock())
        {
            asset->Clear(); // SkeletalMeshAsset 내부 리소스 해제
        }
        m_SkeletalMeshMap.erase(it); // 캐시에서 제거
    }
}


std::shared_ptr<StaticMeshAsset> AssetManager::LoadStaticMesh(ID3D11Device* device, const std::string& path)
{
    AssetKey key(path, EAssetKind::StaticMesh);

    auto it = m_StaticMeshMap.find(key);
    if (it != m_StaticMeshMap.end())
    {
        if (auto exist = it->second.lock())
            return exist;
        else
            m_StaticMeshMap.erase(it);
    }

    auto asset = std::make_shared<StaticMeshAsset>();
    if (asset->LoadFromFBX(device, path))
    {
        m_StaticMeshMap[key] = asset;
        return asset;
    }
    return nullptr;
}

void AssetManager::UnloadStaticMesh(const std::string& path)
{
    AssetKey key(path, EAssetKind::StaticMesh);
    auto it = m_StaticMeshMap.find(key);
    if (it != m_StaticMeshMap.end())
    {
        if (auto asset = it->second.lock())
        {
            asset->Clear();
        }
        m_StaticMeshMap.erase(it);
    }
}

void AssetManager::UnloadAll()
{
    m_SkeletalMeshMap.clear();   // weak_ptr 이므로 자원 자동 해제
    m_StaticMeshMap.clear();      // shared_ptr 해제 → GPU 리소스 해제됨
}