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

	ComPtr<IDXGIFactory4> m_pDXGIFactory;		// DXGI팩토리
	ComPtr<IDXGIAdapter3> m_pDXGIAdapter;		// 비디오카드 정보에 접근 가능한 인터페이스

	// [ 렌더링 파이프라인을 구성하는 필수 객체의 인터페이스 ] 
	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	ComPtr<IDXGISwapChain1> m_pSwapChain;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;
	ComPtr<ID3D11DepthStencilView> m_pDepthStencilView;	// 깊이값 처리를 위한 뎊스스텐실 뷰

	// [ 렌더링 파이프라인 객체 ]
	ComPtr<ID3D11VertexShader> m_pVertexShader; // 정점 셰이더
	ComPtr<ID3D11PixelShader> m_pPixelShader;	// 픽셀 셰이더
	ComPtr<ID3D11InputLayout> m_pInputLayout;	// 입력 레이아웃
	ComPtr<ID3D11Buffer> m_pVertexBuffer;		// 버텍스 버퍼
	ComPtr<ID3D11Buffer> m_pIndexBuffer;		// 인덱스 버퍼
	ComPtr<ID3D11Buffer> m_pConstantBuffer;		// 상수 버퍼 

	// [ 렌더링 파이프라인관련 정보 ]
	UINT m_VertextBufferStride = 0;				// 버텍스 하나의 크기
	UINT m_VertextBufferOffset = 0;				// 버텍스 버퍼의 오프셋
	UINT m_VertexCount = 0;						// 버텍스 개수
	int m_nIndices = 0;							// 인덱스 개수

	// [ 셰이더에 전달할 데이터 ]
	Matrix                m_World1;				// 월드좌표계 공간으로 변환을 위한 행렬.
	Matrix                m_World2;			
	Matrix                m_World3;
	Matrix                m_View;				// 카메라좌표계 공간으로 변환을 위한 행렬.
	Matrix                m_Projection;			// 단위장치좌표계( Normalized Device Coordinate) 공간으로 변환을 위한 행렬.


	// [ 각 큐브의 기본 위치 ]
	Vector3 m_World1Pos    = Vector3(0.0f, 0.0f, 0.0f);
	Vector3 m_World2Offset = Vector3(-4.0f, 0.0f, 0.0f);	// 상대 위치
	Vector3 m_World3Offset = Vector3(-2.0f, 0.0f, 0.0f);	// 상대 위치 


	// [ 배경색 ]
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
	void Render_ImGui();	// Cube, 카메라의 위치 조정 UI

	bool InitD3D();
	void UninitD3D();		

	bool InitScene();		// 쉐이더,버텍스,인덱스
	void UninitScene();

	bool InitImGUI();
	void UninitImGUI();

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};