#include "Material.h"
#include "../Common/Helper.h"

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem> // #include <filesystem>
#include <DirectXTK/WICTextureLoader.h>

using namespace DirectX;

namespace fs = std::experimental::filesystem;

// 기본 텍스처 static 멤버
TextureSRVs Material::s_defaultTextures; // static 구조체

// 텍스처 타입, 인덱스, 출력 경로
// 같은 타입을 여러장 쓰는 경우는 특수한 경우이므로 인덱스 0만 사용 
// GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))

// [ Assimp 머티리얼에서 텍스처 읽어서 DirectX SRV 생성 ]
void Material::InitializeFromAssimpMaterial(ID3D11Device* device, const aiMaterial* material, const std::wstring& textureBasePath)
{
    // default texture 보장
    if (!s_defaultTextures.DiffuseSRV) 
    { 
        CreateDefaultTextures(device); 
    }

    // [ Diffuse Color 읽기 ]
    aiColor4D aiDiffuse(1.0f, 1.0f, 1.0f, 1.0f);
    if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &aiDiffuse))
    {
        DiffuseColor = XMFLOAT4(aiDiffuse.r, aiDiffuse.g, aiDiffuse.b, aiDiffuse.a);
        OutputDebugString((L"[Material Color] DiffuseColor: " +
            std::to_wstring(aiDiffuse.r) + L"," +
            std::to_wstring(aiDiffuse.g) + L"," +
            std::to_wstring(aiDiffuse.b) + L"\n").c_str());
    }
    else
    {
        DiffuseColor = XMFLOAT4(1, 1, 1, 1);
    }

    // [ Specular Color + Shininess 읽기 ]
    aiColor4D aiSpecular(1.0f, 1.0f, 1.0f, 1.0f);
    float shininess = 32.0f;           // 기본값 
    float shininessStrength = 1.0f;    // 보정값

    // Specular 색상
    if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &aiSpecular))
    {
        SpecularColor = XMFLOAT4(aiSpecular.r, aiSpecular.g, aiSpecular.b, aiSpecular.a);
    }
    else
    {
        SpecularColor = XMFLOAT4(1, 1, 1, 1);
    }

    // Shininess (하이라이트 크기)
    if (AI_SUCCESS != aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess))
    {
        shininess = 32.0f;
    }

    // Shininess Strength (하이라이트 강도)
    if (AI_SUCCESS == aiGetMaterialFloat(material, AI_MATKEY_SHININESS_STRENGTH, &shininessStrength))
    {
        shininess *= shininessStrength;
    }

    // 디버그 출력
    OutputDebugString((L"[Material Specular] Color: " +
        std::to_wstring(aiSpecular.r) + L"," +
        std::to_wstring(aiSpecular.g) + L"," +
        std::to_wstring(aiSpecular.b) +
        L" Shininess: " + std::to_wstring(shininess) + L"\n").c_str());

    // 저장
    Shininess = shininess;

    // [2] Diffuse 텍스처 로드
    aiString texPath;
    if (AI_SUCCESS == material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath))
    {
        // 텍스처 경로가 있을 경우
        fs::path fileName = fs::path(texPath.C_Str()).filename();
        FilePathDiffuse = (fs::path(textureBasePath) / fileName).wstring();

        if (FAILED(CreateTextureFromFile(device, FilePathDiffuse.c_str(), m_textures.DiffuseSRV.GetAddressOf())))
        {
            m_textures.DiffuseSRV = s_defaultTextures.DiffuseSRV;
            OutputDebugString(L"[Material] Diffuse texture load failed. Using default.\n");
        }
    }
    else
    {
        // 텍스처 없음 → DiffuseColor로 1x1 텍스처 생성
        unsigned char color[4] = {
            (unsigned char)(DiffuseColor.x * 255),
            (unsigned char)(DiffuseColor.y * 255),
            (unsigned char)(DiffuseColor.z * 255),
            (unsigned char)(DiffuseColor.w * 255)
        };

        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = color;
        init.SysMemPitch = 4;

        ComPtr<ID3D11Texture2D> tex;
        device->CreateTexture2D(&desc, &init, &tex);

        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        device->CreateShaderResourceView(tex.Get(), &srvDesc, &m_textures.DiffuseSRV);

        OutputDebugString(L"[Material] DiffuseColor -> Created 1x1 color texture.\n");
    }

    // [3] 나머지 텍스처 로드 (변경 없음)
    auto LoadTex = [&](aiTextureType type, ComPtr<ID3D11ShaderResourceView>& out, std::wstring& outPath, ComPtr<ID3D11ShaderResourceView> fallback)
        {
            aiString texPath;
            if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS)
            {
                fs::path fileName = fs::path(texPath.C_Str()).filename();
                outPath = (fs::path(textureBasePath) / fileName).wstring();

                if (FAILED(CreateTextureFromFile(device, outPath.c_str(), out.GetAddressOf())))
                    out = fallback;
            }
            else
            {
                out = fallback;
            }
        };

    // 각 텍스처 로드
    // LoadTex(aiTextureType_DIFFUSE, m_textures.DiffuseSRV, FilePathDiffuse, s_defaultTextures.DiffuseSRV);
    LoadTex(aiTextureType_NORMALS, m_textures.NormalSRV, FilePathNormal, s_defaultTextures.NormalSRV);
    LoadTex(aiTextureType_SPECULAR, m_textures.SpecularSRV, FilePathSpecular, s_defaultTextures.SpecularSRV);
    LoadTex(aiTextureType_EMISSIVE, m_textures.EmissiveSRV, FilePathEmissive, s_defaultTextures.EmissiveSRV);
    LoadTex(aiTextureType_OPACITY, m_textures.OpacitySRV, FilePathOpacity, s_defaultTextures.OpacitySRV);
}


// [ 1x1 기본 텍스처 생성 ] TODO : GUI로 조정 가능하도록 하면 좋을듯 
void Material::CreateDefaultTextures(ID3D11Device* device)
{
    if (s_defaultTextures.DiffuseSRV) return; // 이미 생성됨

    unsigned char white[] = { 255,255,255,255 };
    unsigned char gray[] = { 128,128,128,255 };
    unsigned char black[] = { 0,0,0,255 };
    unsigned char normal[] = { 128,128,255,255 };

    auto Create1x1Tex = [&](unsigned char color[4], ComPtr<ID3D11ShaderResourceView>& srv)
    {
        // Texture2D 설정
        D3D11_TEXTURE2D_DESC desc{};
        desc.Width = desc.Height = 1;
        desc.MipLevels = 1;
        desc.ArraySize = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        desc.MiscFlags = 0;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        D3D11_SUBRESOURCE_DATA init{};
        init.pSysMem = color;
        init.SysMemPitch = 4;

        ComPtr<ID3D11Texture2D> tex;
        device->CreateTexture2D(&desc, &init, &tex);

        // ShaderResourceView 생성
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = desc.Format;
        srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;

        device->CreateShaderResourceView(tex.Get(), &srvDesc, &srv);
    };

    Create1x1Tex(white, s_defaultTextures.DiffuseSRV);
    Create1x1Tex(normal, s_defaultTextures.NormalSRV);
    Create1x1Tex(white, s_defaultTextures.SpecularSRV);
    Create1x1Tex(black, s_defaultTextures.EmissiveSRV);
    Create1x1Tex(white, s_defaultTextures.OpacitySRV);
}

const TextureSRVs& Material::GetDefaultTextures()
{
    return s_defaultTextures;
}

void Material::DestroyDefaultTextures()
{
    s_defaultTextures.DiffuseSRV.Reset();
    s_defaultTextures.NormalSRV.Reset();
    s_defaultTextures.SpecularSRV.Reset();
    s_defaultTextures.EmissiveSRV.Reset();
    s_defaultTextures.OpacitySRV.Reset();
}

void Material::Clear()
{
    m_textures.DiffuseSRV.Reset();
    m_textures.NormalSRV.Reset();
    m_textures.SpecularSRV.Reset();
    m_textures.EmissiveSRV.Reset();
    m_textures.OpacitySRV.Reset();
}