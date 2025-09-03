#include "pch.h"
#include "Helper.h"

#include <comdef.h>
#include <d3dcompiler.h>
#include <Directxtk/DDSTextureLoader.h>
#include <Directxtk/WICTextureLoader.h>


// HRESULT �� ���� �޽��� ���ڿ� ��ȯ
LPCWSTR GetComErrorString(HRESULT hr)
{
	_com_error err(hr);
	LPCWSTR errMsg = err.ErrorMessage();
	return errMsg;
}

// HLSL ���̴� ������ 
HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// ����� ����� ���: ������� ���� �ϱ� ���� ����ȭ ��
	dwShaderFlags |= D3DCOMPILE_DEBUG;
	dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif


	ID3DBlob* pErrorBlob = nullptr;
	hr = D3DCompileFromFile(szFileName, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, szEntryPoint, szShaderModel,
		dwShaderFlags, 0, ppBlobOut, &pErrorBlob);

	if (FAILED(hr))
	{
		if (pErrorBlob)
		{
			MessageBoxA(NULL, (char*)pErrorBlob->GetBufferPointer(), "CompileShaderFromFile", MB_OK);
			pErrorBlob->Release();
		}
		return hr;
	}
	if (pErrorBlob) pErrorBlob->Release();

	return S_OK;
}


// �ؽ�ó �ε� 
HRESULT CreateTextureFromFile(ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11ShaderResourceView** textureView)
{
	HRESULT hr = S_OK;

	// DDS ���� �ؽ�ó �õ�
	hr = DirectX::CreateDDSTextureFromFile(d3dDevice, szFileName, nullptr, textureView);
	if (FAILED(hr))
	{
		// WIC ��� �δ� (PNG, JPG, BMP ��)
		hr = DirectX::CreateWICTextureFromFile(d3dDevice, szFileName, nullptr, textureView);
		if (FAILED(hr))
		{
			// ���� �� ���� �޽��� ���
			MessageBoxW(NULL, GetComErrorString(hr), szFileName, MB_OK);
			return hr;
		}
	}
	return S_OK;
}
