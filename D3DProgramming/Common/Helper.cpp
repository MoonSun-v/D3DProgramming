#include "pch.h"
#include "Helper.h"

#include <comdef.h>
#include <d3dcompiler.h>
#include <Directxtk/DDSTextureLoader.h>
#include <Directxtk/WICTextureLoader.h>
#include <dxgidebug.h>
#include <dxgi1_3.h>  // DXGIGetDebugInterface1

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


// 텍스처 로드 
HRESULT CreateTextureFromFile(ID3D11Device* d3dDevice, const wchar_t* szFileName, ID3D11ShaderResourceView** textureView)
{
	HRESULT hr = S_OK;

	// DDS 포맷 텍스처 시도
	hr = DirectX::CreateDDSTextureFromFile(d3dDevice, szFileName, nullptr, textureView);
	if (FAILED(hr))
	{
		// WIC 기반 로더 (PNG, JPG, BMP 등)
		hr = DirectX::CreateWICTextureFromFile(d3dDevice, szFileName, nullptr, textureView);
		if (FAILED(hr))
		{
			// 실패 시 에러 메시지 출력
			MessageBoxW(NULL, GetComErrorString(hr), szFileName, MB_OK);
			return hr;
		}
	}
	return S_OK;
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
