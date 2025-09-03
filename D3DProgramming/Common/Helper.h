#pragma once
#include <wchar.h>
#include <d3d11.h>
#include <exception>
#include <stdio.h>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <stdexcept>
#include <system_error>
#include <vector>
#include <directxtk/SimpleMath.h>
using namespace DirectX::SimpleMath;



// ========== [ 유니코드 wide 문자 버전 로그 매크로 ] ========== 

// 에러 로그 (MessageBoxW를 띄워서 직접 알림)
#define LOG_ERROR(...) \
{ \
    wchar_t buffer[256]; \
    swprintf_s(buffer,256, L"[ERROR] %s:%d - ", __FUNCTIONW__, __LINE__); \
    wchar_t message[256]; \
    swprintf_s(message,256, __VA_ARGS__); \
    wcscat_s(buffer, message); \
    wcscat_s(buffer, L"\n"); \
    MessageBoxW(NULL, buffer, L"LOG_ERROR", MB_OK); \
}

// 경고 로그 (디버그 출력창에 출력)
#define LOG_WARNING(...) \
{ \
    wchar_t buffer[256]; \
    swprintf_s(buffer,256, L"[WARNING] %s:%d - ", __FUNCTIONW__, __LINE__); \
    wchar_t message[256]; \
    swprintf_s(message,256, __VA_ARGS__); \
    wcscat_s(buffer, message); \
    wcscat_s(buffer, L"\n"); \
    OutputDebugStringW(buffer); \
}

// 일반 메시지 로그 (디버그 출력창에 출력)
#define LOG_MESSAGE(...) \
{ \
    wchar_t buffer[256]; \
    swprintf_s(buffer,256, L"[MESSAGE] %s:%d - ", __FUNCTIONW__, __LINE__); \
    wchar_t message[256]; \
    swprintf_s(message,256, __VA_ARGS__); \
    wcscat_s(buffer, message); \
    wcscat_s(buffer, L"\n"); \
    OutputDebugStringW(buffer); \
}


// ========== [ ANSI(char 기반) 문자열 로그 매크로 ] ==========

// ANSI 문자열 버전 에러 로그 (MessageBoxA)
#define LOG_ERRORA(...) \
{ \
    char buffer[256]; \
    sprintf_s(buffer,256, "[ERROR] %s:%d - ", __FUNCTION__, __LINE__); \
    char message[256]; \
    sprintf_s(message,256, __VA_ARGS__); \
    strcat_s(buffer, message); \
    strcat_s(buffer, "\n"); \
    MessageBoxA(NULL, buffer, "LOG_ERROR", MB_OK); \
}

#define LOG_WARNINGA(...) \
{ \
    char buffer[256]; \
    sprintf_s(buffer,256, "[WARNING] %s:%d - ", __FUNCTION__, __LINE__); \
    char message[256]; \
    sprintf_s(message,256, __VA_ARGS__); \
    strcat_s(buffer, message); \
    strcat_s(buffer, "\n"); \
    OutputDebugStringW(buffer); \
}

#define LOG_MESSAGEA(...) \
{ \
    char buffer[256]; \
    sprintf_s(buffer, 256, "[MESSAGE] %s:%d - ", __FUNCTION__, __LINE__); \
    char message[256]; \
    sprintf_s(message, 256, __VA_ARGS__); \
    strcat_s(buffer, message); \
    strcat_s(buffer, "\n"); \
    OutputDebugStringA(buffer); \
}


// ========== [ 안전 해제 매크로 ] ==========

// COM 객체 안전 해제
template <typename T>
void SAFE_RELEASE(T* p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

// 일반 new 객체 안전 해제
template <typename T>
void SAFE_DELETE(T* p)
{
    if (p)
    {
        delete p;
        p = nullptr;
    }
}


// ========== [ 에러 처리 유틸 ] ==========
// ( HR_T(hr) 매크로로 DirectX API 호출 결과(HRESULT) 검사하고, 실패 시 예외 발생 )


// HRESULT → 문자열 변환
LPCWSTR GetComErrorString(HRESULT hr);


// COM 예외 처리 클래스
class com_exception : public std::exception
{
public:
    com_exception(HRESULT hr) : result(hr) {}

    // 예외 메시지 반환
    const char* what() const noexcept override
    {
        static char s_str[64] = {};
        sprintf_s(s_str, "Failure with HRESULT of %08X",
            static_cast<unsigned int>(result));
        return s_str;
    }

private:
    HRESULT result;
};


// HRESULT 에러 검사 후 예외 발생
inline void HR_T(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw com_exception(hr);
    }
}



//------------------------------[ 셰이더 파일 컴파일 ]-------------------------------
// GPU가 바로 HLSL 코드를 이해하지 못하기 때문에 컴파일 필요함
// 
// 방법 1. 런타임 컴파일 ( D3DCompile )
// 방법 2. 사전 컴파일   ( .cso, Compiled Shader Object )
// 
// 현재 방식 : 런타임 컴파일 (D3DCompile)
//--------------------------------------------------------------------------------------

// [ HLSL 셰이더를 파일에서 컴파일 ]
HRESULT CompileShaderFromFile(
    const WCHAR* szFileName,    // 셰이더 파일 이름
    LPCSTR szEntryPoint,        // 진입점 함수 (ex. "main")
    LPCSTR szShaderModel,       // 셰이더 모델 (ex. "ps_5_0")
    ID3DBlob** ppBlobOut        // 결과 셰이더 바이트코드
);     

// [ 텍스처 파일 로드 (DDS / PNG / JPG 등) ]
HRESULT CreateTextureFromFile(
    ID3D11Device* d3dDevice,                // Direct3D 디바이스
    const wchar_t* szFileName,              // 텍스처 파일 이름
    ID3D11ShaderResourceView** textureView  // 출력 리소스 뷰
); 