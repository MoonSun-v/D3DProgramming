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



// ========== [ �����ڵ� wide ���� ���� �α� ��ũ�� ] ========== 

// ���� �α� (MessageBoxW�� ����� ���� �˸�)
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

// ��� �α� (����� ���â�� ���)
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

// �Ϲ� �޽��� �α� (����� ���â�� ���)
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


// ========== [ ANSI(char ���) ���ڿ� �α� ��ũ�� ] ==========

// ANSI ���ڿ� ���� ���� �α� (MessageBoxA)
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


// ========== [ ���� ���� ��ũ�� ] ==========

// COM ��ü ���� ����
template <typename T>
void SAFE_RELEASE(T* p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

// �Ϲ� new ��ü ���� ����
template <typename T>
void SAFE_DELETE(T* p)
{
    if (p)
    {
        delete p;
        p = nullptr;
    }
}


// ========== [ ���� ó�� ��ƿ ] ==========
// ( HR_T(hr) ��ũ�η� DirectX API ȣ�� ���(HRESULT) �˻��ϰ�, ���� �� ���� �߻� )


// HRESULT �� ���ڿ� ��ȯ
LPCWSTR GetComErrorString(HRESULT hr);


// COM ���� ó�� Ŭ����
class com_exception : public std::exception
{
public:
    com_exception(HRESULT hr) : result(hr) {}

    // ���� �޽��� ��ȯ
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


// HRESULT ���� �˻� �� ���� �߻�
inline void HR_T(HRESULT hr)
{
    if (FAILED(hr))
    {
        throw com_exception(hr);
    }
}



//------------------------------[ ���̴� ���� ������ ]-------------------------------
// GPU�� �ٷ� HLSL �ڵ带 �������� ���ϱ� ������ ������ �ʿ���
// 
// ��� 1. ��Ÿ�� ������ ( D3DCompile )
// ��� 2. ���� ������   ( .cso, Compiled Shader Object )
// 
// ���� ��� : ��Ÿ�� ������ (D3DCompile)
//--------------------------------------------------------------------------------------

// [ HLSL ���̴��� ���Ͽ��� ������ ]
HRESULT CompileShaderFromFile(
    const WCHAR* szFileName,    // ���̴� ���� �̸�
    LPCSTR szEntryPoint,        // ������ �Լ� (ex. "main")
    LPCSTR szShaderModel,       // ���̴� �� (ex. "ps_5_0")
    ID3DBlob** ppBlobOut        // ��� ���̴� ����Ʈ�ڵ�
);     

// [ �ؽ�ó ���� �ε� (DDS / PNG / JPG ��) ]
HRESULT CreateTextureFromFile(
    ID3D11Device* d3dDevice,                // Direct3D ����̽�
    const wchar_t* szFileName,              // �ؽ�ó ���� �̸�
    ID3D11ShaderResourceView** textureView  // ��� ���ҽ� ��
); 