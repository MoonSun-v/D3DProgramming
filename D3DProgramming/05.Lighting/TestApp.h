#pragma once
#include <dxgi1_4.h>
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl/client.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <psapi.h>  // PROCESS_MEMORY_COUNTERS_EX ����
#include <string>

#include "../Common/GameApp.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX::SimpleMath;
using namespace DirectX;

class TestApp : public GameApp
{
public:
	TestApp();
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


	// [ �� ť���� �⺻ ��ġ ]
	Vector3 m_World1Pos    = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 m_World2Offset = Vector3(-4.0f, 0.0f, 0.0f);	// ��� ��ġ
	Vector3 m_World3Offset = Vector3(-2.0f, 0.0f, 0.0f);	// ��� ��ġ 


	// [ ���� ]
	Vector4 m_ClearColor = Vector4(0.80f, 0.92f, 1.0f, 1.0f);  //  Light Sky Blue 



	// [ ImGui ]
	bool m_show_another_window = false;
	bool m_show_demo_window = true;
	float m_f;
	int m_counter;


	// [ Camera ]
	float m_CameraPos[3] = { 0.0f, 0.0f, -30.0f };
	float m_CameraFOV = 60.0f;     // degree
	float m_CameraNear = 0.1f;
	float m_CameraFar = 1000.0f;


	bool Initialize() override;
	void Uninitialize() override;
	void Update() override;
	void Render() override;
	void Render_ImGui();	// Cube, ī�޶��� ��ġ ���� UI

	bool InitD3D();
	void UninitD3D();		

	bool InitScene();		// ���̴�,���ؽ�,�ε���
	void UninitScene();

	bool InitImGUI();
	void UninitImGUI();

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};