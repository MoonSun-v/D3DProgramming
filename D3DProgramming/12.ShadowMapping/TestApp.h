#pragma once
#include "../Common/GameApp.h"
#include "../Common/D3DDevice.h"
#include "SkeletalMesh.h"
#include "ConstantBuffer.h"

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
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// MainPass
	ComPtr<ID3D11VertexShader> m_pShadowVS;			// ShadowPass
	ComPtr<ID3D11PixelShader> m_pPixelShader;		
	ComPtr<ID3D11InputLayout> m_pInputLayout;		// MainPass
	ComPtr<ID3D11InputLayout> m_pShadowInputLayout; // ShadowPass)
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;

	// 버퍼
	ComPtr<ID3D11Buffer> m_pConstantBuffer;     // b0 : 상수 버퍼
	ComPtr<ID3D11Buffer> m_pBonePoseBuffer;     // b1 : Bone Pose
	ComPtr<ID3D11Buffer> m_pBoneOffsetBuffer;   // b2 : Bone Offset

	ConstantBuffer cb;

	// Shadow Map 
	D3D11_VIEWPORT						m_ShadowViewport;   
	ComPtr<ID3D11Texture2D>				m_pShadowMap;       
	ComPtr<ID3D11DepthStencilView>		m_pShadowMapDSV;    // 깊이값 기록을 설정하기 위한 객체 
	ComPtr<ID3D11ShaderResourceView>	m_pShadowMapSRV;    // 셰이더에서 깊이 버퍼를 슬롯에 설정하고 사용하기 위한 객체 
	ComPtr<ID3D11SamplerState>			m_pSamplerComparison; // Comparison Sampler
	ComPtr<ID3D11Buffer> m_pShadowCB;			// Shadow 버퍼


	// [ 셰이더에 전달할 데이터 ]
	Matrix                m_World;					// 모델		-> 월드
	Matrix                m_View;					// 월드		-> 카메라
	Matrix                m_Projection;				// 카메라	-> NDC

	// Shadow			
	Matrix                m_ShadowView;					
	Matrix                m_ShadowProjection;

	Vector3 m_ShadowPos;      // Shadow 카메라 위치
	Vector3 m_ShadowLootAt;   // Shadow 카메라가 바라보는 위치

	// [ 오브젝트 ]
	Matrix m_WorldChar;
	float m_CharPos[3] = { 0.0f, -100.0f, 0.0f };
	Vector3 m_CharScale = { 1.0f, 1.0f, 1.0f };     
	
	float rotX = XMConvertToRadians(0.0f);  // 라디안
	float rotY = XMConvertToRadians(0.0f); 
	float rotZ = XMConvertToRadians(0.0f); 

	// [ 배경색 ]
	// Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f); //  Black
	Vector4 m_ClearColor = Vector4(0.80f, 0.92f, 1.0f, 1.0f); // Light Sky 
	// Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.3f, 1.0f); // Navy


	// [ 라이트 정보 ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 라이트 색상
	XMFLOAT4 m_LightDir = XMFLOAT4(0.0f, 0.0f, 1.0f, 0.0f);	  // 라이트 방향


	// [ Camera 설정 값 ]
	float m_CameraPos[3] = { 0.0f, 0.0f, 0.0f };
	float m_CameraNear = 0.1f;
	float m_CameraFar = 2000.0f;

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

	void RenderShadowMap();

public:
	bool InitScene();		
	bool InitImGUI();

	void Render_ImGui();

	void UninitScene();
	void UninitD3D();
	void UninitImGUI();

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};