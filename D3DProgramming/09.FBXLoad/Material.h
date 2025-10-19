#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <assimp/material.h>

using Microsoft::WRL::ComPtr;

struct TextureSRVs
{
    ComPtr<ID3D11ShaderResourceView> DiffuseSRV;
    ComPtr<ID3D11ShaderResourceView> NormalSRV;
    ComPtr<ID3D11ShaderResourceView> SpecularSRV;
    ComPtr<ID3D11ShaderResourceView> EmissiveSRV;
    ComPtr<ID3D11ShaderResourceView> OpacitySRV;
};

class Material
{
private:
    TextureSRVs m_textures;
    static TextureSRVs s_defaultTextures;

public:
    // 파일 경로를 외부에서 확인할 수 있도록 멤버 추가
    std::wstring FilePathDiffuse;
    std::wstring FilePathNormal;
    std::wstring FilePathSpecular;
    std::wstring FilePathEmissive;
    std::wstring FilePathOpacity;

    void InitializeFromAssimpMaterial(ID3D11Device* device, const aiMaterial* material, const std::wstring& textureBasePath);

    const TextureSRVs& GetTextures() const { return m_textures; }

    static void CreateDefaultTextures(ID3D11Device* device);
    static void DestroyDefaultTextures();
    static const TextureSRVs& GetDefaultTextures();
};