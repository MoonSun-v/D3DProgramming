#pragma once
#include "../Common/GameApp.h"
#include "../Common/D3DDevice.h"
#include "SkeletalMesh.h"

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

	SkeletalMesh boxHuman;

private:
	D3DDevice m_D3DDevice;  

public:

	// [ 렌더링 파이프라인 객체 ]
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// 정점 셰이더
	ComPtr<ID3D11PixelShader> m_pPixelShader;		// 픽셀 셰이더
	ComPtr<ID3D11InputLayout> m_pInputLayout;		// 입력 레이아웃
	ComPtr<ID3D11Buffer> m_pConstantBuffer;			// 상수 버퍼 
	ComPtr<ID3D11Buffer> m_pBoneBuffer;             // 본 전용 상수 버퍼 
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;


	// [ 셰이더에 전달할 데이터 ]
	Matrix                m_World;					// 월드 행렬		(모델 → 월드)
	Matrix                m_View;					// 뷰 행렬		(월드 → 카메라)
	Matrix                m_Projection;				// 프로젝션 행렬 (카메라 → NDC)		

	// [ 오브젝트 ]
	Matrix m_WorldChar;
	float m_CharPos[3] = { 0.0f, -150.0f, 0.0f };


	// [ 배경색 ]
	// Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f); //  Black
	// Vector4 m_ClearColor = Vector4(0.80f, 0.92f, 1.0f, 1.0f); // Light Sky 
	Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.3f, 1.0f); // Navy


	// [ 라이트 정보 ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 라이트 색상
	XMFLOAT4 m_LightDir = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);	  // 라이트 방향


	// [ Camera 설정 값 ]
	float m_CameraPos[3] = { 0.0f, 0.0f, 0.0f };
	float m_CameraNear = 0.1f;
	float m_CameraFar = 1000.0f;

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