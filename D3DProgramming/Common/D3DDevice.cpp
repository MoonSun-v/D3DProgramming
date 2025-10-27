#include "D3DDevice.h"

#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <stdexcept>
#include "../Common/Helper.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


bool D3DDevice::Initialize(HWND hWnd, UINT width, UINT height)
{
    HRESULT hr = 0; // 결과값 : DirectX 함수는 HRESULT 반환

    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0
    };
    D3D_FEATURE_LEVEL featureLevel;


	// ------------------------------------------------------
    // [ Device + DeviceContext 생성 ]
    // ------------------------------------------------------
    HR_T(D3D11CreateDevice(
        nullptr,						// 기본 어댑터 사용
        D3D_DRIVER_TYPE_HARDWARE,		// 하드웨어 렌더링 사용
        0,								// 소프트웨어 드라이버 없음
        creationFlags,					// 디버그 플래그
        featureLevels,					// Feature Level 설정 
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,				// SDK 버전
        m_Device.GetAddressOf(),		// 디바이스 반환
        &featureLevel,			        // Feature Level 반환
        m_DeviceContext.GetAddressOf()  // 디바이스 컨택스트 반환 
    ));


    // ------------------------------------------------------
    // [ SwapChain 생성 ]
    // ------------------------------------------------------
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ComPtr<IDXGIFactory2> factory;
    HR_T(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf())));

    // 스왑체인(백버퍼) 설정
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;									// 백버퍼 개수 
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// 백버퍼 포맷: 32비트 RGBA
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 백버퍼의 용도: 렌더타겟 출력
    swapChainDesc.SampleDesc.Count = 1;								// 멀티샘플링(안티에일리어싱) 사용 안함 (기본: 1, 안 씀)
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;				// Recommended for flip models
    swapChainDesc.Stereo = FALSE;									// 3D 비활성화
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// 전체 화면 전환 허용
    swapChainDesc.Scaling = DXGI_SCALING_NONE;						// 창의 크기와 백 버퍼의 크기가 다를 때. 자동 스케일링X 

    HR_T(factory->CreateSwapChainForHwnd(
        m_Device.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        m_SwapChain.GetAddressOf()
    ));


    // ------------------------------------------------------
    // [ 렌더타겟뷰(Render Target View) 생성 ]
    // ------------------------------------------------------
    ComPtr<ID3D11Texture2D> backBufferTexture;
    HR_T(m_SwapChain->GetBuffer(                        // 스왑체인의 0번 버퍼(백버퍼) 가져옴
        0, __uuidof(ID3D11Texture2D), (void**)(backBufferTexture.GetAddressOf())
        ));		
    HR_T(m_Device.Get()->CreateRenderTargetView(        // 백버퍼 텍스처를 기반으로 렌더타겟뷰 생성
        backBufferTexture.Get(), nullptr, m_RenderTargetView.GetAddressOf()
        )); 


    // ------------------------------------------------------
    // [ DepthStencilView 생성 ]
    // ------------------------------------------------------
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1; 
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24비트 깊이 + 8비트 스텐실
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ComPtr<ID3D11Texture2D> TextureDepthStencil;
    HR_T(m_Device.Get()->CreateTexture2D(&depthDesc, nullptr, TextureDepthStencil.GetAddressOf()));
    HR_T(m_Device.Get()->CreateDepthStencilView(TextureDepthStencil.Get(), nullptr, m_DepthStencilView.GetAddressOf()));


    // ------------------------------------------------------
    // [ Viewport 설정 ]
    // ------------------------------------------------------
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;                          // 뷰포트 시작 X
    viewport.TopLeftY = 0;                          // 뷰포트 시작 Y
    viewport.Width = (float)width;                  // 뷰포트 너비
    viewport.Height = (float)height;                // 뷰포트 높이
    viewport.MinDepth = 0.0f;                       // 깊이 버퍼 최소값
    viewport.MaxDepth = 1.0f;                       // 깊이 버퍼 최대값

    m_DeviceContext->RSSetViewports(1, &viewport); // 래스터라이저(RS: Rasterizer) 단계에 뷰포트 설정


    return true;
}


void D3DDevice::Cleanup()
{
    if (m_DeviceContext) m_DeviceContext->ClearState();

	// ComPtr로 자동으로 해제 되지만, 순서를 명시적으로 지정하고 싶다면 Reset() 호출 가능 
    // 해제 순서: 뷰 -> 스왑체인 -> 컨텍스트 -> 디바이스
    m_DepthStencilView.Reset();
    m_RenderTargetView.Reset();
    m_SwapChain.Reset();
    m_DeviceContext.Reset();
    m_Device.Reset();
}


void D3DDevice::BeginFrame(const FLOAT clearColor[4])
{
    // 그릴 대상 설정 (렌더 타겟 & 뎁스스텐실 설정)
    m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());

    // 화면 초기화 (컬러 + 깊이 버퍼)
    m_DeviceContext->ClearRenderTargetView(m_RenderTargetView.Get(), clearColor);
    m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // 뎁스버퍼 1.0f로 초기화.
} 

void D3DDevice::EndFrame()
{
	m_SwapChain->Present(0, 0); //(1: 수직동기화, 0: 옵션 없음)
}