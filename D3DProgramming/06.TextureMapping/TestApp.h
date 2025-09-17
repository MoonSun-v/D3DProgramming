#pragma once
#include <dxgi1_4.h>
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <wrl/client.h>

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <psapi.h>  // PROCESS_MEMORY_COUNTERS_EX 정의
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

	// [ DXGI 객체 ]
	ComPtr<IDXGIFactory4> m_pDXGIFactory;		// DXGI팩토리
	ComPtr<IDXGIAdapter3> m_pDXGIAdapter;		// 비디오카드 정보에 접근 가능한 인터페이스

	// [ 렌더링 파이프라인을 구성하는 필수 객체의 인터페이스 ] 
	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	ComPtr<IDXGISwapChain1> m_pSwapChain;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;	// 깊이값 처리를 위한 뎊스스텐실 뷰

	ComPtr<ID3D11DepthStencilState> m_pDSStateSky;
	ComPtr < ID3D11RasterizerState> pRasterizerState;

	// [ 렌더링 파이프라인 객체 ]
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// 정점 셰이더
	ComPtr<ID3D11VertexShader> m_pVertexShader_Sky;
	ComPtr<ID3D11PixelShader> m_pPixelShader;		// 픽셀 셰이더
	ComPtr<ID3D11PixelShader> m_pPixelShaderSolid;	// 픽셀 셰이더 라이트 표시용
	ComPtr<ID3D11PixelShader> m_pPixelShader_Sky;
	ComPtr<ID3D11InputLayout> m_pInputLayout;		// 입력 레이아웃
	ComPtr<ID3D11InputLayout> m_pInputLayout_Sky;
	ComPtr<ID3D11Buffer> m_pVertexBuffer;			// 버텍스 버퍼
	ComPtr<ID3D11Buffer> m_pIndexBuffer;			// 인덱스 버퍼
	ComPtr<ID3D11Buffer> m_pConstantBuffer;			// 상수 버퍼 
	ComPtr<ID3D11ShaderResourceView> m_pTextureRV;	// 텍스처 리소스 뷰 (큐브)
	ComPtr<ID3D11ShaderResourceView> m_pCubeMap; // 스카이 박스 
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;	// 샘플러 상태

	// [ 렌더링 파이프라인 관련 정보 ]
	UINT m_VertextBufferStride = 0;					// 버텍스 하나의 크기 (바이트 단위)
	UINT m_VertextBufferOffset = 0;					// 버텍스 버퍼의 오프셋
	UINT m_VertexCount = 0;							// 버텍스 개수
	int m_nIndices = 0;								// 인덱스 개수

	// [ 셰이더에 전달할 데이터 ]
	Matrix                m_World;					// 월드 행렬		(모델 → 월드)
	Matrix                m_View;					// 뷰 행렬		(월드 → 카메라)
	Matrix                m_Projection;				// 프로젝션 행렬 (카메라 → NDC)
	// Vector4	m_vMeshColor = { 0.7f, 0.7f, 0.7f, 1.0f };

	// [ 큐브 오브젝트 ]
	Vector3 m_WorldPos	= Vector3(0.0f, 0.0f, 0.0f); // 위치 
	float	m_CubeYaw	= 0.0f;		// Y축 회전
	float	m_CubePitch	= 0.0f;		// X축 회전

	// [ 배경색 ]
	Vector4 m_ClearColor = Vector4(0.80f, 0.92f, 1.0f, 1.0f);  //  Light Sky Blue 

	// [ 라이트 정보 ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);				// 라이트 색상
	XMFLOAT4 m_InitialLightDir = XMFLOAT4(-0.577f, 0.577f, -0.577f, 1.0f);	// 초기 라이트 방향 (대각선)
	XMFLOAT4 m_LightDirEvaluated = XMFLOAT4(0, 0, 0, 0);					// 계산된 라이트 방향

	// [ ImGui ]
	bool m_show_another_window = false;
	bool m_show_demo_window = true;
	float m_f;
	int m_counter;

	// [ Camera ]
	float m_CameraPos[3] = { 0.0f, 0.0f, -20.0f };
	float m_CameraFOV = 60.0f;     // degree 단위 
	float m_CameraNear = 0.1f;
	float m_CameraFar = 1000.0f;



public:

	bool Initialize() override;
	void Uninitialize() override;
	void Update() override;
	void Render() override;
	void Render_ImGui();	// Cube, 카메라의 위치 조정 UI

	bool InitD3D();
	void UninitD3D();		

	bool InitScene();		// 쉐이더,버텍스,인덱스
	void UninitScene();

	bool InitImGUI();
	void UninitImGUI();

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};