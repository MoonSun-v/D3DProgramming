#include "Material.h"
#include "../Common/Helper.h"

#include <experimental/filesystem> // #include <filesystem>
#include <DirectXTK/WICTextureLoader.h>

using namespace DirectX;

namespace fs = std::experimental::filesystem;

// 텍스처 타입, 인덱스, 출력 경로
// 같은 타입을 여러장 쓰는 경우는 특수한 경우이므로 인덱스 0만 사용 
// GetTexture(aiTextureType_DIFFUSE, 0, &texturePath))

void Material::InitializeFromAssimpMaterial(ID3D11Device* device, const aiMaterial* material, const std::wstring& textureBasePath)
{
    aiString texPath;

    // Diffuse 
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS)
    {
        fs::path fullPath = ToWString(std::string(texPath.C_Str()));
        FilePathDiffuse = textureBasePath + fullPath.filename().wstring();
        HR_T(CreateTextureFromFile(device, FilePathDiffuse.c_str(), DiffuseSRV.GetAddressOf()));
    }

    // Normal 
    if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS)
    {
        fs::path fullPath = ToWString(std::string(texPath.C_Str()));
        FilePathNormal = textureBasePath + fullPath.filename().wstring();
        HR_T(CreateTextureFromFile(device, FilePathNormal.c_str(), NormalSRV.GetAddressOf()));
    }
}
