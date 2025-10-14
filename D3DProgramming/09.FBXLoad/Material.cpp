#include "Material.h"
#include "../Common/Helper.h"

#include <experimental/filesystem> // #include <filesystem>
#include <DirectXTK/WICTextureLoader.h>

using namespace DirectX;

namespace fs = std::experimental::filesystem;

// �ؽ�ó Ÿ��, �ε���, ��� ���
// ���� Ÿ���� ������ ���� ���� Ư���� ����̹Ƿ� �ε��� 0�� ��� 
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
