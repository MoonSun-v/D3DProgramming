#pragma once
#include <map>
#include "SkeletalMeshAsset.h"
#include "AssetKey.h"

class AssetManager
{
public:
    // ΩÃ±€≈Ê 
    static AssetManager& Get()
    {
        static AssetManager instance;
        return instance;
    }

    std::map<AssetKey, std::weak_ptr<SkeletalMeshAsset>> m_SkeletalMeshMap;

    std::shared_ptr<SkeletalMeshAsset> LoadSkeletalMesh(ID3D11Device* device, const std::string& filePath);
};

