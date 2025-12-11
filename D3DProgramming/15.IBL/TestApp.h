#pragma once
#include "../Common/GameApp.h"
#include "../Common/D3DDevice.h"
#include "ConstantBuffer.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <psapi.h>  // PROCESS_MEMORY_COUNTERS_EX 정의
#include <string>

#include <DirectXColors.h>
#include <Effects.h>
#include "SkeletalMeshInstance.h"
#include "StaticMeshInstance.h"

#pragma comment (lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX::SimpleMath;
using namespace DirectX;

struct Vertex_Sky
{
	Vector3 Pos;
};

class TestApp : public GameApp
{
public:
	TestApp();
	~TestApp();

	// [ Human ]
	std::shared_ptr<SkeletalMeshAsset> humanAsset;               // 공유 Asset
	std::vector<std::shared_ptr<SkeletalMeshInstance>> m_Humans; // 인스턴스 벡터

	// [ char ]
	std::shared_ptr<StaticMeshAsset> charAsset;
	std::vector<std::shared_ptr<StaticMeshInstance>> m_Chars;

	// [ Plane ]
	std::shared_ptr<StaticMeshAsset> planeAsset;             
	std::vector<std::shared_ptr<StaticMeshInstance>> m_Planes;     


private:
	D3DDevice m_D3DDevice;  

public:

	// [ 렌더링 파이프라인 객체 ]
	ComPtr<ID3D11VertexShader> m_pVertexShader;		// MainPass
	ComPtr<ID3D11PixelShader> m_pPixelShader;		
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
	ComPtr<ID3D11ShaderResourceView> m_pSkyBoxSRV;			// 스카이 박스 
	// (IBL)
	ComPtr<ID3D11ShaderResourceView> m_pIrradianceSRV;		// Diffuse (Irradiance)
	ComPtr<ID3D11ShaderResourceView> m_pPrefilterSRV;		// Specular (Prefilter)
	ComPtr<ID3D11ShaderResourceView> m_pBRDFLUTSRV;			// BRDF LUT
	ComPtr<ID3D11SamplerState> m_pSamplerIBL;       // s2
	ComPtr<ID3D11SamplerState> m_pSamplerIBL_Clamp; // s3


	UINT m_VertextBufferStride_Sky = 0;
	UINT m_VertextBufferOffset_Sky = 0;
	int m_nIndices_Sky = 0;


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
	float manualRoughness = 0.5f;
	bool useIBL = true;


	// [ Main Camera ]
	float m_CameraNear = 0.1f;
	float m_CameraFar = 5000.0f;


	// [ 배경색 ]
	// Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.0f, 1.0f); //  Black
	Vector4 m_ClearColor = Vector4(0.8f, 0.92f, 1.0f, 1.0f); // Light Sky 
	// Vector4 m_ClearColor = Vector4(0.0f, 0.0f, 0.3f, 1.0f); // Navy


	// [ 라이트 정보 ]
	XMFLOAT4 m_LightColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f); // 라이트 색상
	XMFLOAT4 m_LightDir = XMFLOAT4(0.0f, -1.0f, 1.0f, 0.0f);  // 라이트 방향


	// 머터리얼 조절용
	XMFLOAT4 m_MaterialAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	XMFLOAT4 m_MaterialSpecular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	float    m_Shininess = 280.0f;

	// 블린-퐁 조명용
	XMFLOAT4 m_LightAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
	XMFLOAT4 m_LightDiffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);

	// 폰트 
	ImFont* m_UIFont = nullptr;
	ImFont* m_DebugFont = nullptr;

public:
	bool Initialize() override;
	void Uninitialize() override;
	void Update() override;
	void Render() override;

public:
	bool InitScene();
	bool LoadAsset();
	bool InitSkyBox();
	bool InitImGUI();
	
	void UpdateConstantBuffer(const Matrix& world, int isRigid);

	void RenderStaticMesh(StaticMeshInstance& instance);
	void RenderSkeletalMesh(SkeletalMeshInstance& instance);
	void RenderShadowMap();

	void Render_SkyBox();
	void Render_ImGui();

	void UninitScene();
	void UninitImGUI();

	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};