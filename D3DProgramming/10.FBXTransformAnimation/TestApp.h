#pragma once
#include "../Common/GameApp.h"
#include "../Common/D3DDevice.h"
#include "SkeletalMesh.h"

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

	SkeletalMesh boxHuman;

private:
	D3DDevice m_D3DDevice;  

public:

	// [ ������ ���������� ��ü ]
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// ���� ���̴�
	ComPtr<ID3D11PixelShader> m_pPixelShader;		// �ȼ� ���̴�
	ComPtr<ID3D11InputLayout> m_pInputLayout;		// �Է� ���̾ƿ�
	ComPtr<ID3D11Buffer> m_pConstantBuffer;			// ��� ���� 
	ComPtr<ID3D11Buffer> m_pBoneBuffer;             // �� ���� ��� ���� 
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;


	// [ ���̴��� ������ ������ ]
	Matrix                m_World;					// ���� ���		(�� �� ����)
	Matrix                m_View;					// �� ���		(���� �� ī�޶�)
	Matrix                m_Projection;				// �������� ��� (ī�޶� �� NDC)		

	// [ ������Ʈ ]
	Matrix m_WorldChar;
	float m_CharPos[3] = { 0.0f, -150.0f, 0.0f };


	// [ ���� ]
	// Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f); //  Black
	// Vector4 m_ClearColor = Vector4(0.80f, 0.92f, 1.0f, 1.0f); // Light Sky 
	Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.3f, 1.0f); // Navy


	// [ ����Ʈ ���� ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // ����Ʈ ����
	XMFLOAT4 m_LightDir = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);	  // ����Ʈ ����


	// [ Camera ���� �� ]
	float m_CameraPos[3] = { 0.0f, 0.0f, 0.0f };
	float m_CameraNear = 0.1f;
	float m_CameraFar = 1000.0f;

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