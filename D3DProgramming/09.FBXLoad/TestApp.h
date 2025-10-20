#pragma once
#include "../Common/GameApp.h"
#include "StaticMesh.h"
#include "D3DDevice.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <psapi.h>  // PROCESS_MEMORY_COUNTERS_EX 정의
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
	D3DDevice m_D3DDevice;  // 장치 관리 클래스

public:

	// [ DXGI 객체 ]
	// ComPtr<IDXGIFactory4> m_pDXGIFactory;		// DXGI팩토리
	// ComPtr<IDXGIAdapter3> m_pDXGIAdapter;		// 비디오카드 정보에 접근 가능한 인터페이스

	// [ 렌더링 파이프라인 객체 ]
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// 정점 셰이더
	ComPtr<ID3D11PixelShader> m_pPixelShader;		// 픽셀 셰이더
	ComPtr<ID3D11InputLayout> m_pInputLayout;		// 입력 레이아웃
	ComPtr<ID3D11Buffer> m_pConstantBuffer;			// 상수 버퍼 
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;

	// [ 셰이더에 전달할 데이터 ]
	Matrix                m_World;					// 월드 행렬		(모델 → 월드)
	Matrix                m_View;					// 뷰 행렬		(월드 → 카메라)
	Matrix                m_Projection;				// 프로젝션 행렬 (카메라 → NDC)		


	// [ 오브젝트 ]
	// Vector3 m_WorldPos	= Vector3(0.0f, 0.0f, 0.0f); // 위치 
	// float	m_CubeYaw	= 0.0f;		// Y축 회전
	// float	m_CubePitch	= 0.0f;		// X축 회전


	// [ 배경색 ]
	Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f);  //  Black


	// [ 라이트 정보 ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);		// 라이트 색상
	XMFLOAT4 m_InitialLightDir = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);	// 초기 라이트 방향
	XMFLOAT4 m_LightDirEvaluated = XMFLOAT4(0, 0, 0, 0);			// 계산된 라이트 방향


	// [ ImGui ]
	bool m_show_another_window = false;
	bool m_show_demo_window = true;
	float m_f;
	int m_counter;


	// [ Camera ]
	float m_CameraPos[3] = { 0.0f, 0.0f, -700.0f };
	float m_CameraFOV = 60.0f;		// degree 단위 
	float m_CameraNear = 0.1f;
	float m_CameraFar = 1000.0f;
	float m_CameraYaw = 0.0f;		// Y축 회전 
	float m_CameraPitch = 0.0f;		// X축 회전

	// 머터리얼 조절용
	XMFLOAT4 m_MaterialAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	XMFLOAT4 m_MaterialSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	float    m_Shininess = 280.0f;

	// 블린-퐁 조명용
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