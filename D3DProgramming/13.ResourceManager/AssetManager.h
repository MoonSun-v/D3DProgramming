#pragma once
#include <map>
#include "SkeletalMeshAsset.h"
#include "AssetKey.h"

class AssetManager
{
public:
    
    static AssetManager& Get() // ΩÃ±€≈Ê 
    {
        static AssetManager instance;
        return instance;
    }

    std::map<AssetKey, std::weak_ptr<SkeletalMeshAsset>> m_SkeletalMeshMap;

    std::shared_ptr<SkeletalMeshAsset> LoadSkeletalMesh(ID3D11Device* device, const std::string& filePath);
    void UnloadSkeletalMesh(const std::string& filePath);
};

