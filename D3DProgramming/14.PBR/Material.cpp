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
    // 기본 PBR 텍스처 생성
    if (!s_defaultTextures.BaseColorSRV)
    { 
        CreateDefaultTextures(device); 
    }

    // 텍스처 로드 
    auto LoadPBR = [&](const wchar_t* texName, aiTextureType type, ComPtr<ID3D11ShaderResourceView>& outSRV, std::wstring& outPath)
        {
            aiString texPath;

            if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS)
            {
                // 파일 경로 결합
                fs::path fileName = fs::path(texPath.C_Str()).filename();
                outPath = (fs::path(textureBasePath) / fileName).wstring();

                HRESULT hr = CreateTextureFromFile(device, outPath.c_str(),
                    outSRV.ReleaseAndGetAddressOf());

                if (SUCCEEDED(hr))
                {
                    std::wstring msg = L"[Material] " + std::wstring(texName) + L" 텍스처 로딩 성공 → " + outPath;
                    OutputDebugStringW(msg.c_str()); OutputDebugStringW(L"\n");
                }
                else
                {
                    std::wstring msg = L"[Material] " + std::wstring(texName) + L" 텍스처 로딩 실패 → 기본 텍스처 사용";
                    OutputDebugStringW(msg.c_str()); OutputDebugStringW(L"\n");

                    outSRV = nullptr;
                }
            }
            else
            {
                std::wstring msg = L"[Material] " + std::wstring(texName) + L" 텍스처 없음 → 기본 텍스처 사용";
                OutputDebugStringW(msg.c_str()); OutputDebugStringW(L"\n");

                outSRV = nullptr;
            }
        };

    // Assimp PBR 타입 
    LoadPBR(L"BaseColor", aiTextureType_BASE_COLOR, m_textures.BaseColorSRV, FilePathBaseColor);
    LoadPBR(L"Normal", aiTextureType_NORMALS, m_textures.NormalSRV, FilePathNormal);
    LoadPBR(L"Metallic", aiTextureType_METALNESS, m_textures.MetallicSRV, FilePathMetallic);
    LoadPBR(L"Roughness", aiTextureType_DIFFUSE_ROUGHNESS, m_textures.RoughnessSRV, FilePathRoughness);
    LoadPBR(L"Emissive", aiTextureType_EMISSIVE, m_textures.EmissiveSRV, FilePathEmissive);
    LoadPBR(L"Opacity", aiTextureType_OPACITY, m_textures.OpacitySRV, FilePathOpacity);


    // 없는 텍스처는 기본 텍스처 사용
    if (!m_textures.BaseColorSRV)   m_textures.BaseColorSRV = s_defaultTextures.BaseColorSRV;
    if (!m_textures.NormalSRV)      m_textures.NormalSRV = s_defaultTextures.NormalSRV;
    if (!m_textures.MetallicSRV)    m_textures.MetallicSRV = s_defaultTextures.MetallicSRV;
    if (!m_textures.RoughnessSRV)   m_textures.RoughnessSRV = s_defaultTextures.RoughnessSRV;
    if (!m_textures.EmissiveSRV)    m_textures.EmissiveSRV = s_defaultTextures.EmissiveSRV;
    if (!m_textures.OpacitySRV)     m_textures.OpacitySRV = s_defaultTextures.OpacitySRV;
}


// [ 1x1 기본 텍스처 생성 ] TODO : GUI로 조정 가능하도록 하면 좋을듯 
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

    // Normal = (0,0,1) → (128,128,255)
    Make1x1(128, 128, 255, 255, s_defaultTextures.NormalSRV);

    // Metallic = 0
    Make1x1(0, 0, 0, 255, s_defaultTextures.MetallicSRV);

    // Roughness = 255 (거칠기 = 1.0)
    Make1x1(255, 255, 255, 255, s_defaultTextures.RoughnessSRV);

    // Emissive 
    Make1x1(128, 128, 128, 255, s_defaultTextures.EmissiveSRV);

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