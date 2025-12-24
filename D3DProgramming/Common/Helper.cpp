#include "pch.h"
#include "Helper.h"

#include <comdef.h>
#include <d3dcompiler.h>
#include <Directxtk/DDSTextureLoader.h>
#include <Directxtk/WICTextureLoader.h>
#include <dxgidebug.h>
#include <dxgi1_3.h>  // DXGIGetDebugInterface1
#include <DirectXTex.h>
#include <dxgi1_6.h>

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

const wchar_t* DXGIFormatToString(DXGI_FORMAT format)
{
	switch (format)
	{
	case DXGI_FORMAT_UNKNOWN: return L"UNKNOWN";

	case DXGI_FORMAT_R32G32B32A32_FLOAT: return L"R32G32B32A32_FLOAT (HDR)";
	case DXGI_FORMAT_R16G16B16A16_FLOAT: return L"R16G16B16A16_FLOAT (HDR)";
	case DXGI_FORMAT_R11G11B10_FLOAT:     return L"R11G11B10_FLOAT (HDR)";

	case DXGI_FORMAT_R8G8B8A8_UNORM:       return L"R8G8B8A8_UNORM (LDR)";
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return L"R8G8B8A8_UNORM_SRGB (LDR)";

	case DXGI_FORMAT_B8G8R8A8_UNORM:       return L"B8G8R8A8_UNORM (LDR)";
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return L"B8G8R8A8_UNORM_SRGB (LDR)";

	case DXGI_FORMAT_BC1_UNORM:            return L"BC1_UNORM (DXT1, LDR)";
	case DXGI_FORMAT_BC1_UNORM_SRGB:       return L"BC1_UNORM_SRGB (DXT1, LDR)";
	case DXGI_FORMAT_BC3_UNORM:            return L"BC3_UNORM (DXT5, LDR)";
	case DXGI_FORMAT_BC3_UNORM_SRGB:       return L"BC3_UNORM_SRGB (DXT5, LDR)";
	case DXGI_FORMAT_BC6H_UF16:             return L"BC6H_UF16 (HDR)";
	case DXGI_FORMAT_BC6H_SF16:             return L"BC6H_SF16 (HDR)";
	case DXGI_FORMAT_BC7_UNORM:             return L"BC7_UNORM (LDR)";
	case DXGI_FORMAT_BC7_UNORM_SRGB:        return L"BC7_UNORM_SRGB (LDR)";

	default: return L"(Unknown / Unhandled Format)";
	}
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

	swprintf_s(buffer, L"Format     : %s (%d)\n",
		DXGIFormatToString(meta.format),
		meta.format);
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

	swprintf_s(buffer, L"Compressed : %s\n",
		DirectX::IsCompressed(meta.format) ? L"YES" : L"NO");
	OutputDebugStringW(buffer);

	swprintf_s(buffer, L"sRGB       : %s\n",
		DirectX::IsSRGB(meta.format) ? L"YES" : L"NO");
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

bool CheckHDRSupportAndGetMaxNits(float& outMaxNits, DXGI_FORMAT& outFormat)
{
	ComPtr<IDXGIFactory4> pFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	if (FAILED(hr))
	{
		LOG_ERRORA("ERROR: DXGI Factory 생성 실패.\n");
		return false;
	}
	// 2. 주 그래픽 어댑터 (0번) 열거
	ComPtr<IDXGIAdapter1> pAdapter;
	UINT adapterIndex = 0;
	while (pFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		// WARP 어댑터(소프트웨어)를 건너뛰고 주 어댑터만 사용하도록 선택할 수 있습니다.
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapterIndex++;
			pAdapter.Reset();
			continue;
		}
		break;
	}

	if (!pAdapter)
	{
		LOG_ERRORA("ERROR: 유효한 하드웨어 어댑터를 찾을 수 없습니다.\n");
		return false;
	}

	// 3. 주 모니터 출력 (0번) 열거
	ComPtr<IDXGIOutput> pOutput;
	hr = pAdapter->EnumOutputs(0, &pOutput); // 0번 출력
	if (FAILED(hr))
	{
		LOG_ERRORA("ERROR: 주 모니터 출력(Output 0)을 찾을 수 없습니다.\n");
		return false;
	}

	// 4. HDR 정보를 얻기 위해 IDXGIOutput6으로 쿼리
	ComPtr<IDXGIOutput6> pOutput6;
	hr = pOutput.As(&pOutput6);
	if (FAILED(hr))
	{
		printf("INFO: IDXGIOutput6 인터페이스를 얻을 수 없습니다. HDR 정보를 얻을 수 없습니다.\n");
		outMaxNits = 100.0f;
		outFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		return false;
	}

	// 5. DXGI_OUTPUT_DESC1에서 HDR 정보 확인
	DXGI_OUTPUT_DESC1 desc1 = {};
	hr = pOutput6->GetDesc1(&desc1);
	if (FAILED(hr))
	{
		printf("ERROR: GetDesc1 호출 실패.\n");
		return false;
	}

	// 6. HDR 활성화 조건 분석
	bool isHDRColorSpace = (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
	outMaxNits = (float)desc1.MaxLuminance;

	// OS가 HDR을 켰을 때 MaxLuminance는 100 Nits(SDR 기준)를 초과합니다.
	bool isHDRActive = outMaxNits > 100.0f;

	if (isHDRColorSpace && isHDRActive)
	{
		// 최종 판단: HDR 지원 및 OS 활성화
		outFormat = DXGI_FORMAT_R10G10B10A2_UNORM; // HDR 포맷 설정
		printf("SUCCESS: HDR 활성화됨. MaxNits: %.1f, Format: R10G10B10A2_UNORM\n", outMaxNits);
		return true;
	}
	else
	{
		// HDR 지원 안함 또는 OS에서 비활성화
		outMaxNits = 100.0f; // SDR 기본값
		outFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // SDR 포맷 설정
		printf("INFO: HDR 비활성화. MaxNits: 100.0, Format: R8G8B8A8_UNORM\n");
		return false;
	}
	return true;
}