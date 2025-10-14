#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <assimp/material.h>

using Microsoft::WRL::ComPtr;

class Material
{
public:
    std::wstring FilePathDiffuse;
    std::wstring FilePathNormal;

public:
    void InitializeFromAssimpMaterial(ID3D11Device* device, const aiMaterial* material, const std::wstring& textureBasePath);

    ComPtr<ID3D11ShaderResourceView> DiffuseSRV;
    ComPtr<ID3D11ShaderResourceView> NormalSRV;
    // ComPtr<ID3D11ShaderResourceView> SpecularSRV;
    // ComPtr<ID3D11ShaderResourceView> EmissiveSRV;
};