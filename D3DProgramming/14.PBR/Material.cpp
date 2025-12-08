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


// [ PBR 텍스처 로딩 ]
// BaseColor / Normal / Metallic / Roughness / Emissive / Opacity
void Material::InitializeFromAssimpMaterial(ID3D11Device* device, const aiMaterial* material, const std::wstring& textureBasePath)
{
    // 기본 PBR 텍스처 생성 : 한번만 실행 
    if (!s_defaultTextures.BaseColorSRV)
    { 
        CreateDefaultTextures(device); 
    }

    // 텍스처 로드 
    auto TryLoad = [&](aiTextureType type, std::wstring& outPath, ComPtr<ID3D11ShaderResourceView>& outSRV)
        {
            aiString texPath;

            if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS)
            {
                fs::path fileName = fs::path(texPath.C_Str()).filename();
                outPath = (fs::path(textureBasePath) / fileName).wstring();

                HRESULT hr = CreateTextureFromFile(device, outPath.c_str(), outSRV.ReleaseAndGetAddressOf());

                if (SUCCEEDED(hr))
                {
                    OutputDebugStringW((L"[Material] 텍스처 로딩 성공 → " + outPath + L"\n").c_str());
                    return true;
                }
                else
                {
                    OutputDebugStringW((L"[Material] 텍스처 로딩 실패 → " + outPath + L"\n").c_str());
                    outSRV = nullptr;
                }
            }

            return false;
        };

    // ==== ASSIMP TEXTURE SEARCH PRIORITY TABLE ====
   // FBX 는 동시에 여러 타입에 텍스처가 존재할 수도 있음 -> 우선순위대로 검색

   // BaseColor
    bool foundBase =
        TryLoad(aiTextureType_BASE_COLOR, FilePathBaseColor, m_textures.BaseColorSRV) ||
        TryLoad(aiTextureType_DIFFUSE, FilePathBaseColor, m_textures.BaseColorSRV);  // fallback

    // Normal
    bool foundNormal =
        TryLoad(aiTextureType_NORMALS, FilePathNormal, m_textures.NormalSRV) ||
        TryLoad(aiTextureType_HEIGHT, FilePathNormal, m_textures.NormalSRV); // bump-map fallback

    // Metallic
    bool foundMetal =
        TryLoad(aiTextureType_METALNESS, FilePathMetallic, m_textures.MetallicSRV) ||
        TryLoad(aiTextureType_SPECULAR, FilePathMetallic, m_textures.MetallicSRV); // fallback

    // Roughness
    bool foundRough =
        TryLoad(aiTextureType_DIFFUSE_ROUGHNESS, FilePathRoughness, m_textures.RoughnessSRV);

    // Emissive
    bool foundEmi =
        TryLoad(aiTextureType_EMISSIVE, FilePathEmissive, m_textures.EmissiveSRV);

    // Opacity (Alpha)
    bool foundOpacity =
        TryLoad(aiTextureType_OPACITY, FilePathOpacity, m_textures.OpacitySRV);

    // Fallback ANY (FBX는 가끔 UNKNOWN 타입 텍스처를 넣는다)
    if (!foundBase)
        TryLoad(aiTextureType_UNKNOWN, FilePathBaseColor, m_textures.BaseColorSRV);

    // ====== 기본 텍스처 채우기 ========
    //if (!m_textures.BaseColorSRV)   m_textures.BaseColorSRV = s_defaultTextures.BaseColorSRV;
    //if (!m_textures.NormalSRV)      m_textures.NormalSRV = s_defaultTextures.NormalSRV;
    //if (!m_textures.MetallicSRV)    m_textures.MetallicSRV = s_defaultTextures.MetallicSRV;
    //if (!m_textures.RoughnessSRV)   m_textures.RoughnessSRV = s_defaultTextures.RoughnessSRV;
    //if (!m_textures.EmissiveSRV)    m_textures.EmissiveSRV = s_defaultTextures.EmissiveSRV;
    //if (!m_textures.OpacitySRV)     m_textures.OpacitySRV = s_defaultTextures.OpacitySRV;

    if (!m_textures.BaseColorSRV) 
    {
        m_textures.BaseColorSRV = s_defaultTextures.BaseColorSRV; OutputDebugStringW(L"[Material] 기본 BaseColor 텍스처 사용\n");
    }
    if (!m_textures.NormalSRV) 
    {
        m_textures.NormalSRV = s_defaultTextures.NormalSRV; OutputDebugStringW(L"[Material] 기본 Normal 텍스처 사용\n");
    }
    if (!m_textures.MetallicSRV) 
    {
        m_textures.MetallicSRV = s_defaultTextures.MetallicSRV; OutputDebugStringW(L"[Material] 기본 Metallic 텍스처 사용\n");
    }
    if (!m_textures.RoughnessSRV) 
    {
        m_textures.RoughnessSRV = s_defaultTextures.RoughnessSRV; OutputDebugStringW(L"[Material] 기본 Roughness 텍스처 사용\n");
    }
    if (!m_textures.EmissiveSRV) 
    {
        m_textures.EmissiveSRV = s_defaultTextures.EmissiveSRV; OutputDebugStringW(L"[Material] 기본 Emissive 텍스처 사용\n");
    }
    if (!m_textures.OpacitySRV) 
    {
        m_textures.OpacitySRV = s_defaultTextures.OpacitySRV; OutputDebugStringW(L"[Material] 기본 Opacity 텍스처 사용\n");
    }
}


// [ 1x1 기본 텍스처 생성 ]
void Material::CreateDefaultTextures(ID3D11Device* device)
{
    auto Make1x1 = [&](unsigned char r, unsigned char g, unsigned char b, unsigned char a,
        ComPtr<ID3D11ShaderResourceView>& out)
        {
            unsigned char color[4]{ r, g, b, a };

            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = 1;
            desc.Height = 1;
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
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.MipLevels = 1;

            device->CreateShaderResourceView(tex.Get(), &srvDesc, &out);
        };

    // BaseColor = white
    Make1x1(255, 255, 255, 255, s_defaultTextures.BaseColorSRV);

    // Normal = (0,0,1) -> (128,128,255)
    Make1x1(128, 128, 255, 255, s_defaultTextures.NormalSRV);

    // Metallic = 0
    Make1x1(0, 0, 0, 255, s_defaultTextures.MetallicSRV);

    // Roughness = 128 (거칠기 = 1.0)
    Make1x1(255, 255, 255, 255, s_defaultTextures.RoughnessSRV);

    // Emissive 
    Make1x1(0, 0, 0, 255, s_defaultTextures.EmissiveSRV);

    // Opacity 기본값 = 255 (불투명)
    Make1x1(255, 255, 255, 255, s_defaultTextures.OpacitySRV);
}

const TextureSRVs& Material::GetDefaultTextures()
{
    return s_defaultTextures;
}

void Material::DestroyDefaultTextures()
{
    s_defaultTextures.BaseColorSRV.Reset();
    s_defaultTextures.NormalSRV.Reset();
    s_defaultTextures.MetallicSRV.Reset();
    s_defaultTextures.RoughnessSRV.Reset();
    s_defaultTextures.OpacitySRV.Reset();
}

void Material::Clear()
{
    m_textures.BaseColorSRV.Reset();
    m_textures.NormalSRV.Reset();
    m_textures.MetallicSRV.Reset();
    m_textures.RoughnessSRV.Reset();
    m_textures.OpacitySRV.Reset();
}