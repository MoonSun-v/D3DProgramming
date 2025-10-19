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
    // ���� ��θ� �ܺο��� Ȯ���� �� �ֵ��� ��� �߰�
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