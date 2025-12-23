#pragma once
#include "../Common/GameApp.h"
#include "../Common/D3DDevice.h"
#include "../Common/ConstantBuffer.h"
#include "../Common/DebugDraw.h"

#include <directxtk/SimpleMath.h>
#include <directxtk/CommonStates.h> 
#include <directxtk/Effects.h>		

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <psapi.h>  // PROCESS_MEMORY_COUNTERS_EX 정의
#include <string>

#include "../Common/SkeletalMeshInstance.h"
#include "../Common/StaticMeshInstance.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

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

	// [ char ]
	std::shared_ptr<StaticMeshAsset> charAsset;
	std::vector<std::shared_ptr<StaticMeshInstance>> m_Chars;

	// [ Plane ]
	std::shared_ptr<StaticMeshAsset> planeAsset;
	std::vector<std::shared_ptr<StaticMeshInstance>> m_Planes;

	// [ Tree ]
	std::shared_ptr<StaticMeshAsset> treeAsset;
	std::vector<std::shared_ptr<StaticMeshInstance>> m_Trees;

	// [ Vampire ]
	std::shared_ptr<SkeletalMeshAsset> VampireAsset;
	std::vector<std::shared_ptr<SkeletalMeshInstance>> m_Vampires;

	// [ Human ]
	std::shared_ptr<SkeletalMeshAsset> humanAsset;               // 공유 Asset
	std::vector<std::shared_ptr<SkeletalMeshInstance>> m_Humans; // 인스턴스 벡터


	// [ Debug draw ]
	std::unique_ptr<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> m_DebugBatch;
	std::unique_ptr<DirectX::BasicEffect> m_DebugEffect;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_DebugInputLayout;
	std::unique_ptr<DebugDraw> m_DebugDraw;

	DirectX::BoundingFrustum m_ShadowFrustumWS; // Shadow frustum (world space)

	DirectX::SimpleMath::Vector3 m_ShadowCameraPos; // Shadow camera position (디버그용)

	bool m_DrawShadowFrustum = true; // 토글용


private:
	D3DDevice m_D3DDevice;  

public:

	// [ 렌더링 파이프라인 객체 ]
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// MainPass
	// ComPtr<ID3D11PixelShader> m_pPixelShader;		
	ComPtr<ID3D11InputLayout> m_pInputLayout;		
	ComPtr<ID3D11SamplerState> m_pSamplerLinear;

	ComPtr<ID3D11Buffer> m_pConstantBuffer;			// b0 : 상수 버퍼

	// Shadow Map 
	ComPtr<ID3D11VertexShader> m_pShadowVS;			
	ComPtr<ID3D11PixelShader> m_pShadowPS;
	ComPtr<ID3D11InputLayout> m_pShadowInputLayout;
	D3D11_VIEWPORT						m_ShadowViewport;   
	ComPtr<ID3D11Texture2D>				m_pShadowMap;       
	ComPtr<ID3D11DepthStencilView>		m_pShadowMapDSV;		// 깊이값 기록을 설정하기 위한 객체 
	ComPtr<ID3D11ShaderResourceView>	m_pShadowMapSRV;		// 셰이더에서 깊이 버퍼를 슬롯에 설정하고 사용하기 위한 객체 
	ComPtr<ID3D11SamplerState>			m_pSamplerComparison;	// Comparison Sampler
	ComPtr<ID3D11Buffer>				m_pShadowCB;			// Shadow 버퍼

	// Sky Box
	ComPtr<ID3D11VertexShader> m_pVertexShader_Sky;
	ComPtr<ID3D11PixelShader> m_pPixelShader_Sky;
	ComPtr<ID3D11InputLayout> m_pInputLayout_Sky;
	ComPtr<ID3D11Buffer> m_pVertexBuffer_Sky;
	ComPtr<ID3D11Buffer> m_pIndexBuffer_Sky;
	ComPtr<ID3D11DepthStencilState> m_pDSState_Sky;			// 뎁스스텐실 상태   : 스카이 박스
	ComPtr<ID3D11RasterizerState> m_pRasterizerState_Sky;	// 래스터라이저 상태 : 스카이 박스 
	UINT m_VertextBufferStride_Sky = 0;
	UINT m_VertextBufferOffset_Sky = 0;
	int m_nIndices_Sky = 0;

	ComPtr<ID3D11DepthStencilState> m_pDSState_DebugDraw;

	// IBL
	ComPtr<ID3D11ShaderResourceView> m_pIrradianceSRV;		// Diffuse (Irradiance)
	ComPtr<ID3D11ShaderResourceView> m_pPrefilterSRV;		// Specular (Prefilter)
	ComPtr<ID3D11ShaderResourceView> m_pBRDFLUTSRV;			// BRDF LUT
	ComPtr<ID3D11SamplerState> m_pSamplerIBL;       // s2
	ComPtr<ID3D11SamplerState> m_pSamplerIBL_Clamp; // s3
	std::vector<IBLEnvironment> m_IBLSet;
	int currentIBL = 0;  


	// HDR 
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
	float m_ExposureEV = 0.0f;		// 0 = 기본 노출 (2^0 = 1) // TODO: GUI로 조정
	float m_MonitorMaxNits = 0.0f;

	bool m_EnableDistortion = false; 


	// Deferred Shading G-Buffer 
	static const int GBufferCount = 5; // G-Buffer (Position, Normal, Albedo, MR, Emissive)
	ComPtr<ID3D11Texture2D>        m_GBufferTex[GBufferCount] = {};
	ComPtr<ID3D11RenderTargetView> m_GBufferRTV[GBufferCount] = {};	// Geometry Pass에서 기록
	ComPtr<ID3D11ShaderResourceView> m_GBufferSRV[GBufferCount] = {};	// Deferred Lighting에서 읽기
	ComPtr<ID3D11DepthStencilState> m_pDSState_Deferred;
	ComPtr<ID3D11DepthStencilState> m_pDSState_GBuffer;
	ComPtr<ID3D11RasterizerState> m_pRasterSolid;
	ComPtr<ID3D11VertexShader> m_pGBufferVS;
	ComPtr<ID3D11PixelShader> m_pDeferredLightingPS;
	ComPtr<ID3D11PixelShader> m_pGBufferPS;

	D3D11_VIEWPORT m_Viewport;


	// [ 셰이더에 전달할 데이터 ]
	ConstantBuffer cb;
	Matrix                m_World;					// 모델		-> 월드
	Matrix                m_View;					// 월드		-> 카메라
	Matrix                m_Projection;				// 카메라	-> NDC
	Matrix                m_ShadowView;					
	Matrix                m_ShadowProjection;


	// [ Material GUI 조정 ]
	bool useTex_Base = true;
	bool useTex_Metal = true;
	bool useTex_Rough = true;
	bool useTex_Normal = true;

	XMFLOAT4 manualBaseColor = XMFLOAT4(1, 1, 1, 1);
	float manualMetallic = 0.0f;
	float manualRoughness = 0.0f;
	bool useIBL = true;


	// [ Main Camera ]
	float m_CameraNear = 0.3f;
	float m_CameraFar = 5000.0f;


	// [ 라이트 정보 ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 라이트 색상
	XMFLOAT4 m_LightDir = XMFLOAT4(0.5f, -1.0f, -0.2f, 0.0f);  // 라이트 방향
	float m_LightIntensity = 1.0f;

	// 폰트 
	ImFont* m_UIFont = nullptr;
	ImFont* m_DebugFont = nullptr;

public:
	bool Initialize() override;
	void Uninitialize() override;
	void Update() override;
	void Render() override;

public:
	// Initialize
	bool InitScene();
	bool LoadAsset();
	bool InitSkyBox();
	bool InitImGUI();

	// Update
	void UpdateConstantBuffer(const Matrix& world, int isRigid);

	// Render
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


	// Uninitialize
	void UninitScene();
	void UninitImGUI();

	bool CheckHDRSupportAndGetMaxNits(float& outMaxLuminance, DXGI_FORMAT& outFormat);

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};