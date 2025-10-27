#include "D3DDevice.h"

#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <stdexcept>
#include "../Common/Helper.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")


bool D3DDevice::Initialize(HWND hWnd, UINT width, UINT height)
{
    HRESULT hr = 0; // ����� : DirectX �Լ��� HRESULT ��ȯ

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
    // [ Device + DeviceContext ���� ]
    // ------------------------------------------------------
    HR_T(D3D11CreateDevice(
        nullptr,						// �⺻ ����� ���
        D3D_DRIVER_TYPE_HARDWARE,		// �ϵ���� ������ ���
        0,								// ����Ʈ���� ����̹� ����
        creationFlags,					// ����� �÷���
        featureLevels,					// Feature Level ���� 
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION,				// SDK ����
        m_Device.GetAddressOf(),		// ����̽� ��ȯ
        &featureLevel,			        // Feature Level ��ȯ
        m_DeviceContext.GetAddressOf()  // ����̽� ���ý�Ʈ ��ȯ 
    ));


    // ------------------------------------------------------
    // [ SwapChain ���� ]
    // ------------------------------------------------------
    UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
    dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

    ComPtr<IDXGIFactory2> factory;
    HR_T(CreateDXGIFactory2(0, IID_PPV_ARGS(factory.GetAddressOf())));

    // ����ü��(�����) ����
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = 2;									// ����� ���� 
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// ����� ����: 32��Ʈ RGBA
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// ������� �뵵: ����Ÿ�� ���
    swapChainDesc.SampleDesc.Count = 1;								// ��Ƽ���ø�(��Ƽ���ϸ����) ��� ���� (�⺻: 1, �� ��)
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;				// Recommended for flip models
    swapChainDesc.Stereo = FALSE;									// 3D ��Ȱ��ȭ
    swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ��ü ȭ�� ��ȯ ���
    swapChainDesc.Scaling = DXGI_SCALING_NONE;						// â�� ũ��� �� ������ ũ�Ⱑ �ٸ� ��. �ڵ� �����ϸ�X 

    HR_T(factory->CreateSwapChainForHwnd(
        m_Device.Get(),
        hWnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        m_SwapChain.GetAddressOf()
    ));


    // ------------------------------------------------------
    // [ ����Ÿ�ٺ�(Render Target View) ���� ]
    // ------------------------------------------------------
    ComPtr<ID3D11Texture2D> backBufferTexture;
    HR_T(m_SwapChain->GetBuffer(                        // ����ü���� 0�� ����(�����) ������
        0, __uuidof(ID3D11Texture2D), (void**)(backBufferTexture.GetAddressOf())
        ));		
    HR_T(m_Device.Get()->CreateRenderTargetView(        // ����� �ؽ�ó�� ������� ����Ÿ�ٺ� ����
        backBufferTexture.Get(), nullptr, m_RenderTargetView.GetAddressOf()
        )); 


    // ------------------------------------------------------
    // [ DepthStencilView ���� ]
    // ------------------------------------------------------
    D3D11_TEXTURE2D_DESC depthDesc = {};
    depthDesc.Width = width;
    depthDesc.Height = height;
    depthDesc.MipLevels = 1;
    depthDesc.ArraySize = 1; 
    depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24��Ʈ ���� + 8��Ʈ ���ٽ�
    depthDesc.SampleDesc.Count = 1;
    depthDesc.Usage = D3D11_USAGE_DEFAULT;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

    ComPtr<ID3D11Texture2D> TextureDepthStencil;
    HR_T(m_Device.Get()->CreateTexture2D(&depthDesc, nullptr, TextureDepthStencil.GetAddressOf()));
    HR_T(m_Device.Get()->CreateDepthStencilView(TextureDepthStencil.Get(), nullptr, m_DepthStencilView.GetAddressOf()));


    // ------------------------------------------------------
    // [ Viewport ���� ]
    // ------------------------------------------------------
    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;                          // ����Ʈ ���� X
    viewport.TopLeftY = 0;                          // ����Ʈ ���� Y
    viewport.Width = (float)width;                  // ����Ʈ �ʺ�
    viewport.Height = (float)height;                // ����Ʈ ����
    viewport.MinDepth = 0.0f;                       // ���� ���� �ּҰ�
    viewport.MaxDepth = 1.0f;                       // ���� ���� �ִ밪

    m_DeviceContext->RSSetViewports(1, &viewport); // �����Ͷ�����(RS: Rasterizer) �ܰ迡 ����Ʈ ����


    return true;
}


void D3DDevice::Cleanup()
{
    if (m_DeviceContext) m_DeviceContext->ClearState();

	// ComPtr�� �ڵ����� ���� ������, ������ ��������� �����ϰ� �ʹٸ� Reset() ȣ�� ���� 
    // ���� ����: �� -> ����ü�� -> ���ؽ�Ʈ -> ����̽�
    m_DepthStencilView.Reset();
    m_RenderTargetView.Reset();
    m_SwapChain.Reset();
    m_DeviceContext.Reset();
    m_Device.Reset();
}


void D3DDevice::BeginFrame(const FLOAT clearColor[4])
{
    // �׸� ��� ���� (���� Ÿ�� & �������ٽ� ����)
    m_DeviceContext->OMSetRenderTargets(1, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());

    // ȭ�� �ʱ�ȭ (�÷� + ���� ����)
    m_DeviceContext->ClearRenderTargetView(m_RenderTargetView.Get(), clearColor);
    m_DeviceContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0); // �������� 1.0f�� �ʱ�ȭ.
} 

void D3DDevice::EndFrame()
{
	m_SwapChain->Present(0, 0); //(1: ��������ȭ, 0: �ɼ� ����)
}