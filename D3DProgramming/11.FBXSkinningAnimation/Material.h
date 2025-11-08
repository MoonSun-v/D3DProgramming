#pragma once
#include <d3d11.h>
#include <wrl/client.h>
#include <string>
#include <assimp/material.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;
using namespace DirectX;


// 각 머티리얼 텍스처(SRV) 
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
    TextureSRVs m_textures;                 // 현재 Material이 가진 텍스처
    static TextureSRVs s_defaultTextures;   // 기본 텍스처 (모델에 텍스처 없을 때 사용)

public:
    // 텍스처 파일 경로 (확인용)
    std::wstring FilePathDiffuse;
    std::wstring FilePathNormal;
    std::wstring FilePathSpecular;
    std::wstring FilePathEmissive;
    std::wstring FilePathOpacity;

    // Assimp 머티리얼로부터 초기화
    void InitializeFromAssimpMaterial(ID3D11Device* device, const aiMaterial* material, const std::wstring& textureBasePath);

    XMFLOAT4 DiffuseColor = XMFLOAT4(1, 1, 1, 1); // 기본 흰색
    XMFLOAT4 SpecularColor;
    float Shininess;

    // 텍스처 SRV 
    const TextureSRVs& GetTextures() const { return m_textures; }

    // 기본 텍스처 생성/삭제/조회
    static void CreateDefaultTextures(ID3D11Device* device);
    static void DestroyDefaultTextures(); // 기본 텍스처 해제
    static const TextureSRVs& GetDefaultTextures();
    
    void Clear();                         // 현재 Material 텍스처 해제 
};