#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <assimp/material.h>

using Microsoft::WRL::ComPtr;

// �� ��Ƽ���� �ؽ�ó(SRV) 
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
    TextureSRVs m_textures;                 // ���� Material�� ���� �ؽ�ó
    static TextureSRVs s_defaultTextures;   // �⺻ �ؽ�ó (�𵨿� �ؽ�ó ���� �� ���)

public:
    // �ؽ�ó ���� ��� (Ȯ�ο�)
    std::wstring FilePathDiffuse;
    std::wstring FilePathNormal;
    std::wstring FilePathSpecular;
    std::wstring FilePathEmissive;
    std::wstring FilePathOpacity;

    // Assimp ��Ƽ����κ��� �ʱ�ȭ
    void InitializeFromAssimpMaterial(ID3D11Device* device, const aiMaterial* material, const std::wstring& textureBasePath);

    // �ؽ�ó SRV 
    const TextureSRVs& GetTextures() const { return m_textures; }

    // �⺻ �ؽ�ó ����/����/��ȸ
    static void CreateDefaultTextures(ID3D11Device* device);
    static void DestroyDefaultTextures(); // �⺻ �ؽ�ó ����
    static const TextureSRVs& GetDefaultTextures();
    
    void Clear();                         // ���� Material �ؽ�ó ���� 
};