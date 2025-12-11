#include "pch.h"
#include "Helper.h"

#include <comdef.h>
#include <d3dcompiler.h>
#include <Directxtk/DDSTextureLoader.h>
#include <Directxtk/WICTextureLoader.h>
#include <dxgidebug.h>
#include <dxgi1_3.h>  // DXGIGetDebugInterface1
#include <DirectXTex.h>

#pragma comment(lib, "dxguid.lib")  // 꼭 필요!
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")



// HRESULT → 에러 메시지 문자열 변환
LPCWSTR GetComErrorString(HRESULT hr)
{
	_com_error err(hr);
	LPCWSTR errMsg = err.ErrorMessage();
	return errMsg;
}

std::string GetComErrorStringA(HRESULT hr)
{
	_com_error err(hr);
	LPCWSTR wMsg = err.ErrorMessage();

	// 필요한 버퍼 크기 계산
	int len = WideCharToMultiByte(CP_ACP, 0, wMsg, -1, nullptr, 0, nullptr, nullptr);

	std::string msg(len, '\0');
	WideCharToMultiByte(CP_ACP, 0, wMsg, -1, &msg[0], len, nullptr, nullptr);

	return msg; // std::string으로 반환 (내부적으로 LPCSTR과 호환)
}

// HLSL 셰이더 컴파일 
HRESULT CompileShaderFromFile(const WCHAR* szFileName, LPCSTR szEntryPoint, LPCSTR szShaderModel, ID3DBlob** ppBlobOut)
{
	HRESULT hr = S_OK;

	DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
	// 디버그 모드일 경우: 디버깅을 쉽게 하기 위해 최적화 끔
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

void PrintDDSInfo(const wchar_t* file)
{
	DirectX::TexMetadata meta;
	DirectX::ScratchImage img;

	HRESULT hr = DirectX::LoadFromDDSFile(
		file,
		DirectX::DDS_FLAGS_NONE,
		&meta,
		img
	);

	wchar_t buffer[512];

	if (FAILED(hr))
	{
		swprintf_s(buffer, L"[DDS INFO] Failed to read %s\n", file);
		OutputDebugStringW(buffer);
		return;
	}

	OutputDebugStringW(L"\n===== DDS INFO =====\n");

	swprintf_s(buffer, L"File       : %s\n", file);
	OutputDebugStringW(buffer);

	swprintf_s(buffer, L"Format     : %d\n", meta.format);
	OutputDebugStringW(buffer);

	swprintf_s(buffer, L"Width      : %llu\n", meta.width);
	OutputDebugStringW(buffer);

	swprintf_s(buffer, L"Height     : %llu\n", meta.height);
	OutputDebugStringW(buffer);

	swprintf_s(buffer, L"ArraySize  : %llu\n", meta.arraySize);
	OutputDebugStringW(buffer);

	swprintf_s(buffer, L"MipLevels  : %llu\n", meta.mipLevels);
	OutputDebugStringW(buffer);

	swprintf_s(buffer, L"Cubemap    : %s\n", meta.IsCubemap() ? L"YES" : L"NO");
	OutputDebugStringW(buffer);

	OutputDebugStringW(L"=====================\n");
}


// 텍스처 로드 
HRESULT CreateTextureFromFile(ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11ShaderResourceView** textureView)
{
	HRESULT hr = S_OK;

	std::wstring ext = szFileName;

	// 확장자 추출
	size_t dot = ext.find_last_of(L'.');
	std::wstring extension = (dot != std::wstring::npos) ? ext.substr(dot + 1) : L"";

	// 소문자로 변환
	for (auto& c : extension) c = towlower(c);


	// 1. DDS 텍스처 먼저 시도
	if (extension == L"dds")
	{
		PrintDDSInfo(szFileName);

		hr = DirectX::CreateDDSTextureFromFile(d3dDevice, szFileName, nullptr, textureView);
		return hr;
	}

	// 2. TGA 파일이면 DirectXTex 로더 사용
	if (extension == L"tga")
	{
		DirectX::ScratchImage image;
		hr = DirectX::LoadFromTGAFile(szFileName, nullptr, image);
		if (FAILED(hr))
			return hr;

		hr = DirectX::CreateShaderResourceView(
			d3dDevice,
			image.GetImages(),
			image.GetImageCount(),
			image.GetMetadata(),
			textureView);

		return hr;
	}


	// 3. 나머지 포맷 (PNG, JPG 등)
	hr = DirectX::CreateWICTextureFromFile(d3dDevice, szFileName, nullptr, textureView);

	return hr;
}


void CheckDXGIDebug()
{
	IDXGIDebug1* pDebug = nullptr;

	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
	{
		// 현재 살아있는 DXGI/D3D 객체 출력
		pDebug->ReportLiveObjects(
			DXGI_DEBUG_ALL,                 // 모든 DXGI/D3D 컴포넌트
			DXGI_DEBUG_RLO_ALL              // 전체 리포트 옵션
		);

		pDebug->Release();
	}
}
