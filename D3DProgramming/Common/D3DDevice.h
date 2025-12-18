#pragma once
#include <d3d11.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <DirectXMath.h>

using Microsoft::WRL::ComPtr;

class D3DDevice
{
private:
	ComPtr<ID3D11Device>           m_Device;
	ComPtr<ID3D11DeviceContext>    m_DeviceContext;
	ComPtr<IDXGISwapChain1>        m_SwapChain;
	ComPtr<ID3D11RenderTargetView> m_RenderTargetView;
	ComPtr<ID3D11DepthStencilView> m_DepthStencilView;

	D3D11_VIEWPORT viewport;

public:
	// Getter
	ID3D11Device* GetDevice() const { return m_Device.Get(); }
	ID3D11DeviceContext* GetDeviceContext() const { return m_DeviceContext.Get(); }
	IDXGISwapChain1* GetSwapChain() const { return m_SwapChain.Get(); }
	ID3D11RenderTargetView* GetRenderTargetView() const { return m_RenderTargetView.Get(); }
	ID3D11DepthStencilView* GetDepthStencilView() const { return m_DepthStencilView.Get(); }

	const D3D11_VIEWPORT& GetViewport() const { return viewport; }

public:
	bool Initialize(HWND hWnd, UINT width, UINT height, DXGI_FORMAT backBufferFormat);

	void Cleanup();

	void BeginFrame(const FLOAT clearColor[4]);
	void EndFrame();
};