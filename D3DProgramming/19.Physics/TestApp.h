#pragma once

// DirectX / External
#include <directxtk/SimpleMath.h>
#include <directxtk/CommonStates.h> 
#include <directxtk/Effects.h>		
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>

// Common
#include "../Common/GameApp.h"
#include "../Common/D3DDevice.h"
#include "../Common/ConstantBuffer.h"
#include "../Common/DebugDraw.h"
#include "../Common/SkeletalMeshInstance.h"
#include "../Common/StaticMeshInstance.h"

// Helper
#include "PhysicsHelper.h"

// Link Libraries
#pragma comment (lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

// Standard / Windows
#include <psapi.h>  // PROCESS_MEMORY_COUNTERS_EX 정의
#include <string>

// Using
using Microsoft::WRL::ComPtr;
using namespace DirectX::SimpleMath;
using namespace DirectX;

struct Vertex_Sky
{
	Vector3 Pos;
};

struct IBLEnvironment
{
	ComPtr<ID3D11ShaderResourceView> skyboxSRV;
	ComPtr<ID3D11ShaderResourceView> irradianceSRV;
	ComPtr<ID3D11ShaderResourceView> prefilterSRV;
	ComPtr<ID3D11ShaderResourceView> brdfLutSRV;
};

class TestApp : public GameApp
{
public:
	TestApp();
	~TestApp();

private:
	// -------- Static ----------
	PxRigidStatic* m_Plane = nullptr;
	PxRigidStatic* m_CharAsset = nullptr;
	PxRigidStatic* m_TreeAsset = nullptr;
	PxRigidStatic* m_Human1 = nullptr;

	// -------- Dynamic ----------
	PxRigidDynamic* m_Human2 = nullptr;

	// -------- Controller ----------
	PxControllerManager* m_ControllerMgr = nullptr;
	PxController* m_Human3 = nullptr;

	float m_MoveSpeed = 5.0f;

public:
	// -------------------------------
	// Assets (공유 리소스)
	// -------------------------------
	std::shared_ptr<StaticMeshAsset>   planeAsset;
	std::shared_ptr<StaticMeshAsset>   charAsset;
	std::shared_ptr<StaticMeshAsset>   treeAsset;
	std::shared_ptr<SkeletalMeshAsset> humanAsset;
	std::shared_ptr<SkeletalMeshAsset> CharacterAsset;

	// -------------------------------
	// Instances (씬 오브젝트)
	// -------------------------------
	std::vector<std::shared_ptr<StaticMeshInstance>>   m_StaticMeshes;
	std::vector<std::shared_ptr<SkeletalMeshInstance>> m_SkeletalMeshes;


	// ------------------------------------------------------------
	// [ Debug Draw ]
	// ------------------------------------------------------------
	std::unique_ptr<PrimitiveBatch<VertexPositionColor>> m_DebugBatch;
	std::unique_ptr<BasicEffect> m_DebugEffect;
	ComPtr<ID3D11InputLayout> m_DebugInputLayout;
	std::unique_ptr<DebugDraw> m_DebugDraw;

	ComPtr<ID3D11DepthStencilState> m_pDSState_DebugDraw;

	BoundingFrustum m_ShadowFrustumWS;
	Vector3 m_ShadowCameraPos;			// 디버그용
	bool m_DrawShadowFrustum = true;	// 토글용


private:
	D3DDevice m_D3DDevice;  

public:

	// ------------------------------------------------------------
	// [ 공통 파이프라인 객체 ]
	// ------------------------------------------------------------
	ComPtr<ID3D11InputLayout> m_pInputLayout;		
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;
	ComPtr<ID3D11Buffer> m_pConstantBuffer;			// b0 


	// ------------------------------------------------------------
	// [ Shadow Map ]
	// ------------------------------------------------------------
	ComPtr<ID3D11VertexShader> m_pShadowVS;			
	ComPtr<ID3D11PixelShader> m_pShadowPS;
	ComPtr<ID3D11InputLayout> m_pShadowInputLayout;
	D3D11_VIEWPORT m_ShadowViewport{};
	ComPtr<ID3D11Texture2D>				m_pShadowMap;       
	ComPtr<ID3D11DepthStencilView>		m_pShadowMapDSV;		
	ComPtr<ID3D11ShaderResourceView>	m_pShadowMapSRV;		
	ComPtr<ID3D11SamplerState>			m_pSamplerComparison;	
	ComPtr<ID3D11Buffer>				m_pShadowCB;			// b3


	// ------------------------------------------------------------
	// [ Sky Box ]
	// ------------------------------------------------------------
	ComPtr<ID3D11VertexShader> m_pVertexShader_Sky;
	ComPtr<ID3D11PixelShader> m_pPixelShader_Sky;
	ComPtr<ID3D11InputLayout> m_pInputLayout_Sky;
	ComPtr<ID3D11Buffer> m_pVertexBuffer_Sky;
	ComPtr<ID3D11Buffer> m_pIndexBuffer_Sky;
	ComPtr<ID3D11DepthStencilState> m_pDSState_Sky;			
	ComPtr<ID3D11RasterizerState> m_pRasterizerState_Sky;	

	UINT m_VertextBufferStride_Sky = 0;
	UINT m_VertextBufferOffset_Sky = 0;
	int m_nIndices_Sky = 0;


	// ------------------------------------------------------------
	// [ IBL ]
	// ------------------------------------------------------------
	ComPtr<ID3D11SamplerState> m_pSamplerIBL;       // s2
	ComPtr<ID3D11SamplerState> m_pSamplerIBL_Clamp; // s3
	std::vector<IBLEnvironment> m_IBLSet;
	int currentIBL = 0;  


	// ------------------------------------------------------------
	// [ HDR / ToneMap ]
	// ------------------------------------------------------------
	ComPtr<ID3D11Buffer> m_ToneMapCB;
	ComPtr<ID3D11VertexShader> m_pToneMapVS;
	ComPtr<ID3D11PixelShader> m_pToneMapPS_HDR;
	ComPtr<ID3D11PixelShader> m_pToneMapPS_LDR;

	ComPtr<ID3D11Texture2D> m_HDRSceneTex;
	ComPtr<ID3D11RenderTargetView> m_HDRSceneRTV;
	ComPtr<ID3D11ShaderResourceView> m_HDRSceneSRV;
	
	DXGI_FORMAT m_SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	bool m_isHDRSupported = false;
	bool m_forceLDR = false;		// WinMain에서 설정 

	float m_ExposureEV = 0.0f;		// 0 = 기본 노출 (2^0 = 1)
	float m_MonitorMaxNits = 0.0f;


	// ------------------------------------------------------------
	// [ Deferred Shading : G-Buffer ]
	// ------------------------------------------------------------
	static const int GBufferCount = 5; // G-Buffer (Position, Normal, Albedo, MR, Emissive)
	ComPtr<ID3D11Texture2D>        m_GBufferTex[GBufferCount] = {};
	ComPtr<ID3D11RenderTargetView> m_GBufferRTV[GBufferCount] = {};		// Geometry Pass에서 기록
	ComPtr<ID3D11ShaderResourceView> m_GBufferSRV[GBufferCount] = {};	// Deferred Lighting에서 읽기
	
	ComPtr<ID3D11DepthStencilState> m_pDSState_GBuffer;
	ComPtr<ID3D11DepthStencilState> m_pDSState_Deferred;
	ComPtr<ID3D11VertexShader> m_pGBufferVS;
	ComPtr<ID3D11PixelShader> m_pGBufferPS;
	ComPtr<ID3D11PixelShader> m_pDeferredLightingPS;
	
	D3D11_VIEWPORT m_Viewport{};

	
	// ------------------------------------------------------------
	// [ Constant / Camera / Light ]
	// ------------------------------------------------------------
	ConstantBuffer cb;

	Matrix                m_World;					// 모델		-> 월드
	Matrix                m_View;					// 월드		-> 카메라
	Matrix                m_Projection;				// 카메라	-> NDC
	Matrix                m_ShadowView;					
	Matrix                m_ShadowProjection;

	float m_CameraNear = 0.3f;
	float m_CameraFar = 5000.0f;

	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); 
	XMFLOAT4 m_LightDir = XMFLOAT4(0.5f, -1.0f, -0.2f, 0.0f); 
	float m_LightIntensity = 1.0f;



	// ------------------------------------------------------------
	// [ GUI / Material Control ]
	// ------------------------------------------------------------
	bool useTex_Base = true;
	bool useTex_Metal = true;
	bool useTex_Rough = true;
	bool useTex_Normal = true;
	bool useIBL = true;

	XMFLOAT4 manualBaseColor = XMFLOAT4(1, 1, 1, 1);
	float manualMetallic = 0.0f;
	float manualRoughness = 0.0f;
	
	// 폰트 
	ImFont* m_UIFont = nullptr;
	ImFont* m_DebugFont = nullptr;
	ImFont* TitleFont = nullptr;

	// [ (효과) 화면 왜곡 ]
	bool m_EnableDistortion = false;

	// [ 애니메이션 ]
	bool bIdle;
	bool bDance1;
	bool bDance2;


public:
	// ------------------------------------------------------------
	// Lifecycle
	// ------------------------------------------------------------
	bool Initialize() override;
	void Uninitialize() override;
	void Update() override;
	void Render() override;

public:
	// ------------------------------------------------------------
	// Initialize
	// ------------------------------------------------------------
	bool InitScene();
	bool LoadAsset();
	bool InitSkyBox();
	bool InitImGUI();

	// ------------------------------------------------------------
	// Render Pass
	// ------------------------------------------------------------
	void Render_BeginGBuffer();        
	void Render_GBufferGeometry();
	void Render_DeferredLighting();

	void Render_ShadowMap();
	void Render_BeginSceneHDR();
	void Render_SkyBox();
	void RenderStaticMesh(StaticMeshInstance& instance);
	void RenderSkeletalMesh(SkeletalMeshInstance& instance);
	void Render_BeginBackBuffer();
	void Render_ToneMapping();
	void Render_DebugDraw();
	void Render_ImGui();

	// ------------------------------------------------------------
	// Uninitialize
	// ------------------------------------------------------------
	void UninitScene();
	void UninitImGUI();

	// ------------------------------------------------------------
	// Utility
	// ------------------------------------------------------------
	void UpdateConstantBuffer(const Matrix& world, int isRigid);
	// bool CheckHDRSupportAndGetMaxNits(float& outMaxLuminance, DXGI_FORMAT& outFormat);

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};