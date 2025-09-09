#pragma once
#include <dxgi1_4.h>
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl/client.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

#include <string>

#include "../Common/GameApp.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX::SimpleMath;
using namespace DirectX;

class TestApp : public GameApp
{
public:
	TestApp(HINSTANCE hInstance);
	~TestApp();

	ComPtr<IDXGIFactory4> m_pDXGIFactory;		// DXGI���丮
	ComPtr<IDXGIAdapter3> m_pDXGIAdapter;		// ����ī�� ������ ���� ������ �������̽�

	// [ ������ ������������ �����ϴ� �ʼ� ��ü�� �������̽� ] 
	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	ComPtr<IDXGISwapChain1> m_pSwapChain;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;	// ���̰� ó���� ���� �X�����ٽ� ��

	// [ ������ ���������� ��ü ]
	ComPtr<ID3D11VertexShader> m_pVertexShader; // ���� ���̴�
	ComPtr<ID3D11PixelShader> m_pPixelShader;	// �ȼ� ���̴�
	ComPtr<ID3D11InputLayout> m_pInputLayout;	// �Է� ���̾ƿ�
	ComPtr<ID3D11Buffer> m_pVertexBuffer;		// ���ؽ� ����
	ComPtr<ID3D11Buffer> m_pIndexBuffer;		// �ε��� ����
	ComPtr<ID3D11Buffer> m_pConstantBuffer;		// ��� ���� 

	// [ ������ ���������ΰ��� ���� ]
	UINT m_VertextBufferStride = 0;				// ���ؽ� �ϳ��� ũ��
	UINT m_VertextBufferOffset = 0;				// ���ؽ� ������ ������
	UINT m_VertexCount = 0;						// ���ؽ� ����
	int m_nIndices = 0;							// �ε��� ����

	// [ ���̴��� ������ ������ ]
	Matrix                m_World1;				// ������ǥ�� �������� ��ȯ�� ���� ���.
	Matrix                m_World2;			
	Matrix                m_World3;
	Matrix                m_View;				// ī�޶���ǥ�� �������� ��ȯ�� ���� ���.
	Matrix                m_Projection;			// ������ġ��ǥ��( Normalized Device Coordinate) �������� ��ȯ�� ���� ���.


	// [ ���� ]
	Vector4 m_ClearColor = Vector4(0.80f, 0.92f, 1.0f, 1.0f);  //  Light Sky Blue 

	// [ ImGui ]
	bool m_show_another_window = false;
	bool m_show_demo_window = true;
	float m_f;
	int m_counter;


	bool Initialize(UINT Width, UINT Height) override;
	void Uninitialize() override;
	void Update() override;
	void Render() override;

	bool InitD3D();
	void UninitD3D();		

	bool InitScene();		// ���̴�,���ؽ�,�ε���
	void UninitScene();

	bool InitImGUI();
	void UninitImGUI();

	void GetDisplayMemoryInfo(std::string& out);
	void GetVirtualMemoryInfo(std::string& out);
};