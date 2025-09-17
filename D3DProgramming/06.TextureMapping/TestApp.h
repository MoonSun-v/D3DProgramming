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

	// [ DXGI ��ü ]
	ComPtr<IDXGIFactory4> m_pDXGIFactory;		// DXGI���丮
	ComPtr<IDXGIAdapter3> m_pDXGIAdapter;		// ����ī�� ������ ���� ������ �������̽�

	// [ ������ ������������ �����ϴ� �ʼ� ��ü�� �������̽� ] 
	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	ComPtr<IDXGISwapChain1> m_pSwapChain;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;	// ���̰� ó���� ���� �X�����ٽ� ��

	ComPtr<ID3D11DepthStencilState> m_pDSStateSky;
	ComPtr < ID3D11RasterizerState> pRasterizerState;

	// [ ������ ���������� ��ü ]
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// ���� ���̴�
	ComPtr<ID3D11VertexShader> m_pVertexShader_Sky;
	ComPtr<ID3D11PixelShader> m_pPixelShader;		// �ȼ� ���̴�
	ComPtr<ID3D11PixelShader> m_pPixelShaderSolid;	// �ȼ� ���̴� ����Ʈ ǥ�ÿ�
	ComPtr<ID3D11PixelShader> m_pPixelShader_Sky;
	ComPtr<ID3D11InputLayout> m_pInputLayout;		// �Է� ���̾ƿ�
	ComPtr<ID3D11InputLayout> m_pInputLayout_Sky;
	ComPtr<ID3D11Buffer> m_pVertexBuffer;			// ���ؽ� ����
	ComPtr<ID3D11Buffer> m_pIndexBuffer;			// �ε��� ����
	ComPtr<ID3D11Buffer> m_pConstantBuffer;			// ��� ���� 
	ComPtr<ID3D11ShaderResourceView> m_pTextureRV;	// �ؽ�ó ���ҽ� �� (ť��)
	ComPtr<ID3D11ShaderResourceView> m_pCubeMap; // ��ī�� �ڽ� 
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;	// ���÷� ����

	// [ ������ ���������� ���� ���� ]
	UINT m_VertextBufferStride = 0;					// ���ؽ� �ϳ��� ũ�� (����Ʈ ����)
	UINT m_VertextBufferOffset = 0;					// ���ؽ� ������ ������
	UINT m_VertexCount = 0;							// ���ؽ� ����
	int m_nIndices = 0;								// �ε��� ����

	// [ ���̴��� ������ ������ ]
	Matrix                m_World;					// ���� ���		(�� �� ����)
	Matrix                m_View;					// �� ���		(���� �� ī�޶�)
	Matrix                m_Projection;				// �������� ��� (ī�޶� �� NDC)
	// Vector4	m_vMeshColor = { 0.7f, 0.7f, 0.7f, 1.0f };

	// [ ť�� ������Ʈ ]
	Vector3 m_WorldPos	= Vector3(0.0f, 0.0f, 0.0f); // ��ġ 
	float	m_CubeYaw	= 0.0f;		// Y�� ȸ��
	float	m_CubePitch	= 0.0f;		// X�� ȸ��

	// [ ���� ]
	Vector4 m_ClearColor = Vector4(0.80f, 0.92f, 1.0f, 1.0f);  //  Light Sky Blue 

	// [ ����Ʈ ���� ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);				// ����Ʈ ����
	XMFLOAT4 m_InitialLightDir = XMFLOAT4(-0.577f, 0.577f, -0.577f, 1.0f);	// �ʱ� ����Ʈ ���� (�밢��)
	XMFLOAT4 m_LightDirEvaluated = XMFLOAT4(0, 0, 0, 0);					// ���� ����Ʈ ����

	// [ ImGui ]
	bool m_show_another_window = false;
	bool m_show_demo_window = true;
	float m_f;
	int m_counter;

	// [ Camera ]
	float m_CameraPos[3] = { 0.0f, 0.0f, -20.0f };
	float m_CameraFOV = 60.0f;     // degree ���� 
	float m_CameraNear = 0.1f;
	float m_CameraFar = 1000.0f;



public:

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