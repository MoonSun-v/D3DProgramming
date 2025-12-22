#pragma once
#include <string> 

enum class EAssetKind : uint8_t 
{
    Texture2D, TextureCube, StaticMesh, SkeletalMesh, Skeleton, Animation, Material, ShaderSrc, Other
};

struct AssetKey 
{
    std::string pathNorm;
    EAssetKind     kind;

    AssetKey() = default;
    AssetKey(const std::string& path, EAssetKind k) : pathNorm(path), kind(k) {}

    friend bool operator==(const AssetKey& a, const AssetKey& b)
    {
        return a.kind == b.kind && a.pathNorm == b.pathNorm;
    }

    // map에서 key로 사용하려면 비교 연산자 필요
    bool operator<(const AssetKey& other) const
    {
        if (kind != other.kind) return kind < other.kind;
        return pathNorm < other.pathNorm;
    }
};