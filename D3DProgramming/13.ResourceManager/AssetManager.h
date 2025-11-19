#pragma once
#include <map>
#include "SkeletalMeshAsset.h"
#include "StaticMeshAsset.h"
#include "AssetKey.h"
#include <unordered_map>

class AssetManager
{
public:
    
    static AssetManager& Get() // 싱글톤 
    {
        static AssetManager instance;
        return instance;
    }

    std::map<AssetKey, std::weak_ptr<SkeletalMeshAsset>> m_SkeletalMeshMap;
    std::unordered_map<std::string, std::shared_ptr<StaticMeshAsset>> m_StaticMeshes;

    std::shared_ptr<SkeletalMeshAsset> LoadSkeletalMesh(ID3D11Device* device, const std::string& filePath);
    void UnloadSkeletalMesh(const std::string& filePath);

    std::shared_ptr<StaticMeshAsset> LoadStaticMesh(ID3D11Device* device, const std::string& path)
    {
        auto it = m_StaticMeshes.find(path);
        if (it != m_StaticMeshes.end())
            return it->second;

        auto asset = std::make_shared<StaticMeshAsset>();
        if (asset->LoadFromFBX(device, path))
        {
            m_StaticMeshes[path] = asset;
            return asset;
        }
        return nullptr;
    }

    void UnloadStaticMesh(const std::string& path)
    {
        m_StaticMeshes.erase(path); // shared_ptr 참조가 0이면 실제 해제
    }
};

