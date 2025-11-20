#pragma once
#include <map>
#include "SkeletalMeshAsset.h"
#include "StaticMeshAsset.h"
#include "AssetKey.h"
#include <unordered_map>

class AssetManager
{
public:
    
    static AssetManager& Get() // ΩÃ±€≈Ê 
    {
        static AssetManager instance;
        return instance;
    }

    std::map<AssetKey, std::weak_ptr<SkeletalMeshAsset>> m_SkeletalMeshMap;
    std::map<AssetKey, std::weak_ptr<StaticMeshAsset>> m_StaticMeshMap;

    std::shared_ptr<SkeletalMeshAsset> LoadSkeletalMesh(ID3D11Device* device, const std::string& filePath);
    void UnloadSkeletalMesh(const std::string& filePath);

    std::shared_ptr<StaticMeshAsset> LoadStaticMesh(ID3D11Device* device, const std::string& path);
    void UnloadStaticMesh(const std::string& path);

    void UnloadAll();
};

