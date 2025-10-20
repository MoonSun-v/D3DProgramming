#pragma once
#include "../Common/GameApp.h"
#include "StaticMesh.h"
#include "D3DDevice.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <psapi.h>  // PROCESS_MEMORY_COUNTERS_EX ����
#include <string>

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

	StaticMesh treeMesh, charMesh, zeldaMesh;

private:
	D3DDevice m_D3DDevice;  // ��ġ ���� Ŭ����

public:

	// [ DXGI ��ü ]
	// ComPtr<IDXGIFactory4> m_pDXGIFactory;		// DXGI���丮
	// ComPtr<IDXGIAdapter3> m_pDXGIAdapter;		// ����ī�� ������ ���� ������ �������̽�

	// [ ������ ���������� ��ü ]
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// ���� ���̴�
	ComPtr<ID3D11PixelShader> m_pPixelShader;		// �ȼ� ���̴�
	ComPtr<ID3D11InputLayout> m_pInputLayout;		// �Է� ���̾ƿ�
	ComPtr<ID3D11Buffer> m_pConstantBuffer;			// ��� ���� 
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;

	// [ ���̴��� ������ ������ ]
	Matrix                m_World;					// ���� ���		(�� �� ����)
	Matrix                m_View;					// �� ���		(���� �� ī�޶�)
	Matrix                m_Projection;				// �������� ��� (ī�޶� �� NDC)		


	// [ ������Ʈ ]
	// Vector3 m_WorldPos	= Vector3(0.0f, 0.0f, 0.0f); // ��ġ 
	// float	m_CubeYaw	= 0.0f;		// Y�� ȸ��
	// float	m_CubePitch	= 0.0f;		// X�� ȸ��


	// [ ���� ]
	Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);  //  Black


	// [ ����Ʈ ���� ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);		// ����Ʈ ����
	XMFLOAT4 m_InitialLightDir = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);	// �ʱ� ����Ʈ ����
	XMFLOAT4 m_LightDirEvaluated = XMFLOAT4(0, 0, 0, 0);			// ���� ����Ʈ ����


	// [ ImGui ]
	bool m_show_another_window = false;
	bool m_show_demo_window = true;
	float m_f;
	int m_counter;


	// [ Camera ]
	float m_CameraPos[3] = { 0.0f, 0.0f, -700.0f };
	float m_CameraFOV = 60.0f;		// degree ���� 
	float m_CameraNear = 0.1f;
	float m_CameraFar = 1000.0f;
	float m_CameraYaw = 0.0f;		// Y�� ȸ�� 
	float m_CameraPitch = 0.0f;		// X�� ȸ��

	// ���͸��� ������
	XMFLOAT4 m_MaterialAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	XMFLOAT4 m_MaterialSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	float    m_Shininess = 280.0f;

	// ��-�� �����
	XMFLOAT4 m_LightAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	XMFLOAT4 m_LightDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

public:
	bool Initialize() override;
	void Uninitialize() override;
	void Update() override;
	void Render() override;

public:
	bool InitScene();		
	bool InitImGUI();

	void Render_ImGui();

	void UninitScene();
	void UninitD3D();
	void UninitImGUI();

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};