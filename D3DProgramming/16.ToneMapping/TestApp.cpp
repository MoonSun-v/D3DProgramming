#include "TestApp.h"
#include "AssetManager.h"

#include <string> 
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <Directxtk/DDSTextureLoader.h>
#include <windows.h>
#include <dxgi1_6.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "Psapi.lib")


TestApp::TestApp() : GameApp() 
{ 
}

TestApp::~TestApp() { }

bool TestApp::Initialize()
{
	__super::Initialize();

	
	m_isHDRSupported = CheckHDRSupportAndGetMaxNits(m_MonitorMaxNits, m_SwapChainFormat);

	if (!m_forceLDR && m_isHDRSupported)
		m_SwapChainFormat = DXGI_FORMAT_R10G10B10A2_UNORM;	// HDR
	else
		m_SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;		// LDR


	if (!m_D3DDevice.Initialize(m_hWnd, m_ClientWidth, m_ClientHeight, m_SwapChainFormat)) return false;
	if (!InitScene())	return false;
	if (!LoadAsset())	return false;
	if (!InitSkyBox())  return false;
	if (!InitImGUI())	return false;


	// ------------------------------------
	// DebugDraw 초기화
	// ------------------------------------
	m_DebugBatch = std::make_unique<DirectX::PrimitiveBatch<VertexPositionColor>>( m_D3DDevice.GetDeviceContext() );

	m_DebugEffect = std::make_unique<DirectX::BasicEffect>( m_D3DDevice.GetDevice() );
	m_DebugEffect->SetVertexColorEnabled(true);

	void const* shaderByteCode;
	size_t byteCodeLength;
	m_DebugEffect->GetVertexShaderBytecode(&shaderByteCode, &byteCodeLength);

	m_D3DDevice.GetDevice()->CreateInputLayout(
		DirectX::VertexPositionColor::InputElements,
		DirectX::VertexPositionColor::InputElementCount,
		shaderByteCode,
		byteCodeLength,
		m_DebugInputLayout.GetAddressOf()
	);

	m_DebugDraw = std::make_unique<DebugDraw>();

	return true;
}

void TestApp::Uninitialize()
{
	UninitScene();      // 리소스 해제 
	UninitImGUI();
	m_D3DDevice.Cleanup();
	CheckDXGIDebug();	// DirectX 리소스 누수 체크
}


// [ 리소스 로드 (Asset) ]
bool TestApp::LoadAsset()
{
	// -------------- [ Static Mesh Asset 생성 ] --------------
	// 1. Plane 
	planeAsset = AssetManager::Get().LoadStaticMesh(m_D3DDevice.GetDevice(), "../Resource/Plane.fbx");
	auto instance_plane = std::make_shared<StaticMeshInstance>(); // StaticMeshInstance 생성 후 Asset 연결
	instance_plane->SetAsset(planeAsset);
	instance_plane->transform.position = { 0, -15, 0 };
	instance_plane->transform.scale = { 0.5, 1, 0.5 };
	m_Planes.push_back(instance_plane);

	// 2. char
	charAsset = AssetManager::Get().LoadStaticMesh(m_D3DDevice.GetDevice(), "../Resource/char/char.fbx");
	auto instance_char = std::make_shared<StaticMeshInstance>(); 
	instance_char->SetAsset(charAsset);
	instance_char->transform.position = { 0, 0, -90 };
	instance_char->transform.rotation = { 0, XMConvertToRadians(90), 0 }; // TODO : Transform 라디안 변환
	instance_char->transform.scale = { 1.0f, 1.0f, 1.0f };
	m_Chars.push_back(instance_char);


	// -------------- [ Skeletal Mesh Asset 생성 ] --------------
	// 1. Human
	humanAsset = AssetManager::Get().LoadSkeletalMesh(m_D3DDevice.GetDevice(), "../Resource/Skeletal/DancingHuman.fbx"); 
	auto instance_human = std::make_shared<SkeletalMeshInstance>();
	instance_human->SetAsset(m_D3DDevice.GetDevice(), humanAsset);
	instance_human->transform.position = { 0, 0, 10 };
	instance_human->transform.rotation = { 0, XMConvertToRadians(45), 0 };
	instance_human->transform.scale = { 1.0, 1.0, 1.0 };
	m_Humans.push_back(instance_human);

	// 2. Vampire
	VampireAsset = AssetManager::Get().LoadSkeletalMesh(m_D3DDevice.GetDevice(), "../Resource/Skeletal/Vampire_SkinningTest.fbx");
	auto instance_Vampire = std::make_shared<SkeletalMeshInstance>();
	instance_Vampire->SetAsset(m_D3DDevice.GetDevice(), VampireAsset);
	instance_Vampire->transform.position = { 0, 0, 100 };
	instance_Vampire->transform.rotation = { 0, XMConvertToRadians(45), 0 };
	instance_Vampire->transform.scale = { 1.0, 1.0, 1.0 };
	m_Vampires.push_back(instance_Vampire);


	// -------------- [ SkyBox 리소스 ] --------------
	m_IBLSet.resize(3);

	// Sky 
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/Sky/Sky_EnvHDR.dds", m_IBLSet[0].skyboxSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/Sky/Sky_DiffuseHDR.dds", m_IBLSet[0].irradianceSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/Sky/Sky_SpecularHDR.dds", m_IBLSet[0].prefilterSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/Sky/Sky_Brdf.dds", m_IBLSet[0].brdfLutSRV.ReleaseAndGetAddressOf()));

	// InDoor 
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/cubemapEnvHDR.dds", m_IBLSet[1].skyboxSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/cubemapDiffuseHDR.dds", m_IBLSet[1].irradianceSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/cubemapSpecularHDR.dds", m_IBLSet[1].prefilterSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/cubemapBrdf.dds", m_IBLSet[1].brdfLutSRV.ReleaseAndGetAddressOf()));

	// OutDoor 
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/_OutDoor/OutDoor_EnvHDR.dds", m_IBLSet[2].skyboxSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/_OutDoor/OutDoor_DiffuseHDR.dds", m_IBLSet[2].irradianceSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/_OutDoor/OutDoor_SpecularHDR.dds", m_IBLSet[2].prefilterSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(m_D3DDevice.GetDevice(), L"../Resource/SkyBox/_OutDoor/OutDoor_Brdf.dds", m_IBLSet[2].brdfLutSRV.ReleaseAndGetAddressOf()));

	return true; 
}


void TestApp::Update()
{
	__super::Update();

	float deltaTime = TimeSystem::m_Instance->DeltaTime();
	float totalTime = TimeSystem::m_Instance->TotalTime();

	m_Camera.GetViewMatrix(m_View);			// View 행렬 갱신

	// [ 오브젝트 업데이트 ] 
	// m_Planes[0]->transform.position.y = -10.0f;
	// m_Planes[0]->transform.scale = { 0.5f, 1.0f, 0.5f };


	// 인스턴스들 Update 호출 (업데이트된 world 전달)
	for (size_t i = 0; i < m_Humans.size(); i++)
	{
		m_Humans[i]->Update(deltaTime);
	}
	for (size_t i = 0; i < m_Vampires.size(); i++)
	{
		m_Vampires[i]->Update(deltaTime);
	}

	// ---------------------------------------------
	// [ Shadow 카메라 위치 계산 (원근 투영) ] 
	// ---------------------------------------------

	// [ ShadowLookAt ]
	float shadowDistance = 1.0f; // 카메라 앞쪽 거리
	Vector3 shadowLookAt = m_Camera.m_Position + m_Camera.GetForward() * shadowDistance;

	// [ ShadowPos ] 라이트 방향을 기준으로 카메라 위치 계산 
	float shadowBackOffset = 1000.0f;  // 라이트가 어느 정도 떨어진 곳에서 바라보게
	Vector3 lightDir = XMVector3Normalize(Vector3(m_LightDir.x, m_LightDir.y, m_LightDir.z));
	Vector3 shadowPos = m_Camera.m_Position + (-lightDir * shadowBackOffset);

	// Shadow 카메라 위치 저장 (DebugDraw용)
	m_ShadowCameraPos = shadowPos;

	// [ Shadow View ]
	m_ShadowView = XMMatrixLookAtLH(shadowPos, shadowLookAt, Vector3(0.0f, 1.0f, 0.0f));

	// Projection 행렬 (Perspective 원근 투영) : fov, aspect, nearZ, farZ
	m_ShadowProjection = XMMatrixPerspectiveFovLH(1.5f/*XM_PIDIV4*/, m_ShadowViewport.Width / (FLOAT)m_ShadowViewport.Height, 300.0f, 9000.f);


	// ------------------------------------
	// Shadow Frustum (World Space)
	// ------------------------------------
	DirectX::BoundingFrustum::CreateFromMatrix( m_ShadowFrustumWS, m_ShadowProjection );

	// View inverse로 월드 변환
	Matrix invShadowView = m_ShadowView.Invert();
	m_ShadowFrustumWS.Transform(m_ShadowFrustumWS, invShadowView);
}     

void TestApp::UpdateConstantBuffer(const Matrix& world, int isRigid)
{
	cb.mWorld = XMMatrixTranspose(world);
	cb.mView = XMMatrixTranspose(m_View);
	cb.mProjection = XMMatrixTranspose(m_Projection);
	cb.vLightDir = m_LightDir;
	cb.vLightColor = m_LightColor;
	cb.vEyePos = XMFLOAT4(m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z, 1.0f);
	cb.gIsRigid = isRigid;

	cb.gMetallicMultiplier = useTex_Metal ? XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f) : XMFLOAT4(manualMetallic, 0.0f, 0.0f, 0.0f); // 텍스처 사용 여부에 따라 GUI 값 적용
	cb.gRoughnessMultiplier = useTex_Rough ? XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f) : XMFLOAT4(manualRoughness, 0.0f, 0.0f, 0.0f);
	cb.manualBaseColor = manualBaseColor; 

	cb.useTexture_BaseColor = useTex_Base;
	cb.useTexture_Metallic = useTex_Metal;
	cb.useTexture_Roughness = useTex_Rough;
	cb.useTexture_Normal = useTex_Normal;
	cb.useIBL = useIBL; // useIBL ? 1 : 0;

	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_D3DDevice.GetDeviceContext()->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
}

// --------------------------------------------
//	HDR 파이프라인 렌더링 
// --------------------------------------------
//	[1] ShadowMap Pass
//	[2] Scene HDR Pass(R16G16B16A16_FLOAT)
//		- SkyBox
//		- PBR + IBL
//	[3] ToneMapping Pass(HDR → LDR)
//	[4] UI / Debug Pass(LDR 위에: 톤매핑 이후에)
//	[5] Present

void TestApp::Render()
{
	// --------------------------------------------
	// [1] DepthOnly Pass 렌더링 (ShadowMap) 
	// --------------------------------------------
	Render_ShadowMap(); 


	// --------------------------------------------
	// [2] HDR Scene Pass (R16G16B16A16_FLOAT) 
	// --------------------------------------------
	
	Render_BeginSceneHDR(); // HDR RT 바인딩 
	

	Render_SkyBox();  // SkyBox 렌더링 

	auto* context = m_D3DDevice.GetDeviceContext();
	context->IASetInputLayout(m_pInputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	context->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
	context->PSSetSamplers(1, 1, m_pSamplerComparison.GetAddressOf());
	context->PSSetSamplers(2, 1, m_pSamplerIBL.GetAddressOf());        // samLinearIBL
	context->PSSetSamplers(3, 1, m_pSamplerIBL_Clamp.GetAddressOf());  // samClampIBL

	// ShadowMap SRV 바인딩
	context->PSSetShaderResources(6, 1, m_pShadowMapSRV.GetAddressOf());

	// IBL 리소스 바인딩
	if (useIBL)
	{
		context->PSSetShaderResources(11, 1, m_IBLSet[currentIBL].irradianceSRV.GetAddressOf());
		context->PSSetShaderResources(12, 1, m_IBLSet[currentIBL].prefilterSRV.GetAddressOf());
		context->PSSetShaderResources(13, 1, m_IBLSet[currentIBL].brdfLutSRV.GetAddressOf());
	}
	else
	{
		ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
		context->PSSetShaderResources(11, 1, nullSRV);
		context->PSSetShaderResources(12, 1, nullSRV);
		context->PSSetShaderResources(13, 1, nullSRV);
	}

	// Mesh 렌더링 : Static Mesh Instance 
	for (size_t i = 0; i < m_Planes.size(); i++) { RenderStaticMesh(*m_Planes[i]); }
	for (size_t i = 0; i < m_Chars.size(); i++) { RenderStaticMesh(*m_Chars[i]); }

	// Mesh 렌더링 : Skeletal Mesh Instance 
	for (size_t i = 0; i < m_Humans.size(); i++) { RenderSkeletalMesh(*m_Humans[i]); }
	for (size_t i = 0; i < m_Vampires.size(); i++) { RenderSkeletalMesh(*m_Vampires[i]); }



	// --------------------------------------------
	// [3] Tone Mapping Pass
	// --------------------------------------------

	Render_BeginBackBuffer(); // BackBuffer RTV
	Render_ToneMapping();     // Fullscreen Quad


	// --------------------------------------------
	// [4] UI / Debug (LDR)
	// --------------------------------------------
	Render_ImGui();		// UI 렌더링
	Render_DebugDraw(); // Debug Draw 렌더링


	// --------------------------------------------
	// [5] Present 
	// --------------------------------------------
	m_D3DDevice.EndFrame(); 
}

void TestApp::RenderSkeletalMesh(SkeletalMeshInstance& instance)
{
	UpdateConstantBuffer(instance.GetWorld(), 0); // SkeletalMesh

	instance.Render(m_D3DDevice.GetDeviceContext(), m_pSamplerLinear.Get(), 0); 
}

void TestApp::RenderStaticMesh(StaticMeshInstance& instance)
{
	UpdateConstantBuffer(instance.GetWorld(), 1); // StaticMesh

	instance.Render(m_D3DDevice.GetDeviceContext(), m_pSamplerLinear.Get());
}


// ---------------------------------------
// Tone Mapping 결과를 BackBuffer(LDR) 로 출력
// ToneMapping 패스는 Depth 필요 없음
// ---------------------------------------
void TestApp::Render_BeginBackBuffer()
{
	auto* context = m_D3DDevice.GetDeviceContext();

	ID3D11RenderTargetView* rtvs[] = { m_D3DDevice.GetRenderTargetView() }; // GetBackBufferRTV
	context->OMSetRenderTargets(1, rtvs, m_D3DDevice.GetDepthStencilView());

	const float clearColor[4] = { 1, 0, 1, 1 };
	context->ClearRenderTargetView(rtvs[0], clearColor);

	// Viewport 동일
	D3D11_VIEWPORT vp{};
	vp.Width = (float)m_ClientWidth;
	vp.Height = (float)m_ClientHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	context->RSSetViewports(1, &vp);
}


// ---------------------------------------
// HDR SceneTexture → LDR BackBuffer 
// Exposure + ToneMapping + Gamma
// ---------------------------------------
void TestApp::Render_ToneMapping()
{
	auto* context = m_D3DDevice.GetDeviceContext();

	context->OMSetDepthStencilState(nullptr, 0);

	context->IASetInputLayout(nullptr); 
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(m_pToneMapVS.Get(), nullptr, 0);

	// context->PSSetShader(m_pToneMapPS_HDR.Get(), nullptr, 0);
	// context->PSSetShader(m_pToneMapPS_LDR.Get(), nullptr, 0);
	switch (m_SwapChainFormat)
	{
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		context->PSSetShader(m_pToneMapPS_HDR.Get(), nullptr, 0);
		break;
	default:
		context->PSSetShader(m_pToneMapPS_LDR.Get(), nullptr, 0);
		break;
	}

	// HDR SceneTexture SRV
	context->PSSetShaderResources(7, 1, m_HDRSceneSRV.GetAddressOf());
	context->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	ToneMapConstantBuffer toneMapCB;

	// Exposure
	toneMapCB.Exposure = m_ExposureEV;
	toneMapCB.MaxHDRNits = 1000.0f; // HDR10 기준
	context->UpdateSubresource(m_ToneMapCB.Get(), 0, nullptr, &toneMapCB, 0, 0);
	context->PSSetConstantBuffers(4, 1, m_ToneMapCB.GetAddressOf());

	// Draw fullscreen
	context->Draw(3, 0);

	// SRV Unbind
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	context->PSSetShaderResources(7, 1, nullSRV);
}


// ---------------------------------------
// SceneColor를 HDR(R16G16B16A16_FLOAT) 로 렌더
// PBR + IBL 결과를 clamp 없이 저장
// ---------------------------------------
void TestApp::Render_BeginSceneHDR()
{
	// HDR RT + 기존 DepthStencil
	ID3D11RenderTargetView* rtvs[] = { m_HDRSceneRTV.Get() };
	m_D3DDevice.GetDeviceContext()->OMSetRenderTargets(1, rtvs, m_D3DDevice.GetDepthStencilView());

	// Clear
	const float clearColor[4] = { 0, 0, 0, 1 };
	m_D3DDevice.GetDeviceContext()->ClearRenderTargetView(m_HDRSceneRTV.Get(), clearColor);
	m_D3DDevice.GetDeviceContext()->ClearDepthStencilView(
		m_D3DDevice.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

	// Viewport (BackBuffer와 동일)
	D3D11_VIEWPORT vp{};
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	vp.Width = (float)m_ClientWidth;
	vp.Height = (float)m_ClientHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	m_D3DDevice.GetDeviceContext()->RSSetViewports(1, &vp);
}

void TestApp::Render_SkyBox()
{
	// [ 스카이박스 렌더링 전 (깊이 테스트, 래스터라이저 상태 변경) ]
	m_D3DDevice.GetDeviceContext()->OMSetDepthStencilState(m_pDSState_Sky.Get(), 1);
	m_D3DDevice.GetDeviceContext()->RSSetState(m_pRasterizerState_Sky.Get());

	// 설정 
	m_D3DDevice.GetDeviceContext()->IASetInputLayout(m_pInputLayout_Sky.Get());
	m_D3DDevice.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_D3DDevice.GetDeviceContext()->IASetVertexBuffers(0, 1, m_pVertexBuffer_Sky.GetAddressOf(), &m_VertextBufferStride_Sky, &m_VertextBufferOffset_Sky);
	m_D3DDevice.GetDeviceContext()->IASetIndexBuffer(m_pIndexBuffer_Sky.Get(), DXGI_FORMAT_R16_UINT, 0);

	m_D3DDevice.GetDeviceContext()->VSSetShader(m_pVertexShader_Sky.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetShader(m_pPixelShader_Sky.Get(), nullptr, 0);

	m_D3DDevice.GetDeviceContext()->PSSetShaderResources(10, 1, m_IBLSet[currentIBL].skyboxSRV.GetAddressOf());

	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_D3DDevice.GetDeviceContext()->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());


	// Draw
	m_D3DDevice.GetDeviceContext()->DrawIndexed(m_nIndices_Sky, 0, 0);

	// [ 스카이박스 그린 후, 기본 상태로 복원 ]
	m_D3DDevice.GetDeviceContext()->OMSetDepthStencilState(nullptr, 0);
	m_D3DDevice.GetDeviceContext()->RSSetState(nullptr);
}

// -----------------------
// Shadow Map 렌더링
// -----------------------
void TestApp::Render_ShadowMap()
{
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	m_D3DDevice.GetDeviceContext()->PSSetShaderResources(6, 1, nullSRV);

	auto* context = m_D3DDevice.GetDeviceContext();

	// ShadowMap 초기화
	context->ClearDepthStencilView(m_pShadowMapDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	context->OMSetRenderTargets(0, nullptr, m_pShadowMapDSV.Get());
	context->RSSetViewports(1, &m_ShadowViewport);

	// b3 Shadow CB 업데이트
	ShadowConstantBuffer shadowCB;
	shadowCB.mLightView = XMMatrixTranspose(m_ShadowView);
	shadowCB.mLightProjection = XMMatrixTranspose(m_ShadowProjection);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(3, 1, m_pShadowCB.GetAddressOf());

	// b0 업데이트
	context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	// Shadow용 셰이더
	context->IASetInputLayout(m_pShadowInputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->VSSetShader(m_pShadowVS.Get(), nullptr, 0);
	context->PSSetShader(m_pShadowPS.Get(), nullptr, 0);

	context->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	auto RenderShadowStatic = [&](StaticMeshInstance& mesh, const Matrix& world)
		{
			cb.mWorld = XMMatrixTranspose(world);
			cb.gIsRigid = 1;
			context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

			shadowCB.mWorld = XMMatrixTranspose(world);
			context->UpdateSubresource(m_pShadowCB.Get(), 0, nullptr, &shadowCB, 0, 0);

			mesh.RenderShadow(context);
		};

	auto RenderShadowSkeletal = [&](SkeletalMeshInstance& instance, const Matrix& world)
		{
			cb.mWorld = XMMatrixTranspose(world);
			cb.gIsRigid = 0;
			context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

			shadowCB.mWorld = XMMatrixTranspose(world);
			context->UpdateSubresource(m_pShadowCB.Get(), 0, nullptr, &shadowCB, 0, 0);

			instance.RenderShadow(context, 0);
		};


	// [ Static / Skeletal 렌더 ]

	for (size_t i = 0; i < m_Humans.size(); i++)
	{
		RenderShadowSkeletal(*m_Humans[i], m_Humans[i]->GetWorld());
	}
	for (size_t i = 0; i < m_Vampires.size(); i++)
	{
		RenderShadowSkeletal(*m_Vampires[i], m_Vampires[i]->GetWorld());
	}
	for (size_t i = 0; i < m_Planes.size(); i++)
	{
		RenderShadowStatic(*m_Planes[i], m_Planes[i]->GetWorld());
	}
	for (size_t i = 0; i < m_Chars.size(); i++)
	{
		RenderShadowStatic(*m_Chars[i], m_Chars[i]->GetWorld());
	}

	// RenderTarget / Viewport 복원
	context->RSSetViewports(1, &m_D3DDevice.GetViewport());
	ID3D11RenderTargetView* rtv = m_D3DDevice.GetRenderTargetView();
	context->OMSetRenderTargets(1, &rtv, m_D3DDevice.GetDepthStencilView());
}


// [ Scene 초기화 ] 
bool TestApp::InitScene()
{
	HRESULT hr = 0;                       // DirectX 함수 결과값
	ID3D10Blob* errorMessage = nullptr;   // 셰이더 컴파일 에러 메시지 버퍼	

	// ---------------------------------------------------------------
	// 버텍스 셰이더(Vertex Shader) 컴파일 및 생성
	// ---------------------------------------------------------------
	ComPtr<ID3DBlob> vertexShaderBuffer; 
	HR_T(CompileShaderFromFile(L"../Shaders/16.VertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));

	ComPtr<ID3DBlob> ShadowVSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/16.ShadowVertexShader.hlsl", "ShadowVS", "vs_4_0", ShadowVSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(ShadowVSBuffer->GetBufferPointer(), ShadowVSBuffer->GetBufferSize(), NULL, m_pShadowVS.GetAddressOf()));

	ComPtr<ID3DBlob> ToneVSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/16.ToneVertexShader.hlsl", "main", "vs_4_0", ToneVSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(ToneVSBuffer->GetBufferPointer(), ToneVSBuffer->GetBufferSize(), NULL, m_pToneMapVS.GetAddressOf()));



	// --------------------------------------------------------------
	// 입력 레이아웃(Input Layout) 생성
	//---------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },	// POSITION : float3 (12바이트)
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },	// NORMAL   : float3 (12바이트, 오프셋 12)
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },		// TEXCOORD : float2 (8바이트, 오프셋 24)
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // uint4
		{ "BLENDWEIGHT",  0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // float4
	};
	HR_T(m_D3DDevice.GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));

	D3D11_INPUT_ELEMENT_DESC shadowLayout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 72, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	HR_T(m_D3DDevice.GetDevice()->CreateInputLayout(shadowLayout, ARRAYSIZE(shadowLayout), ShadowVSBuffer->GetBufferPointer(), ShadowVSBuffer->GetBufferSize(), m_pShadowInputLayout.GetAddressOf()));


	// ---------------------------------------------------------------
	// 픽셀 셰이더(Pixel Shader) 컴파일 및 생성
	// ---------------------------------------------------------------
	ComPtr<ID3DBlob> pixelShaderBuffer; 
	HR_T(CompileShaderFromFile(L"../Shaders/16.PixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));

	ComPtr<ID3DBlob> ShadowPSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/16.ShadowPixelShader.hlsl", "ShadowPS", "ps_4_0", ShadowPSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(ShadowPSBuffer->GetBufferPointer(), ShadowPSBuffer->GetBufferSize(), NULL, m_pShadowPS.GetAddressOf()));

	ComPtr<ID3DBlob> TonePSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/16.TonePixelShader_HDR.hlsl", "main", "ps_4_0", TonePSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(TonePSBuffer->GetBufferPointer(), TonePSBuffer->GetBufferSize(), NULL, m_pToneMapPS_HDR.GetAddressOf()));

	// ComPtr<ID3DBlob> TonePSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/16.TonePixelShader_LDR.hlsl", "main", "ps_4_0", TonePSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(TonePSBuffer->GetBufferPointer(), TonePSBuffer->GetBufferSize(), NULL, m_pToneMapPS_LDR.GetAddressOf()));

	// ---------------------------------------------------------------
	// 상수 버퍼(Constant Buffer) 생성
	// ---------------------------------------------------------------
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&bd, nullptr, m_pConstantBuffer.GetAddressOf()));


	// ---------------------------------------------------------------
	// Shadow Map 생성 
	// ---------------------------------------------------------------
	
	// Shadow Map 뷰포트 설정
	m_ShadowViewport.TopLeftX = 0.0f;
	m_ShadowViewport.TopLeftY = 0.0f;
	m_ShadowViewport.Width = 8192.0f; 
	m_ShadowViewport.Height = 8192.0f;
	m_ShadowViewport.MinDepth = 0.0f;
	m_ShadowViewport.MaxDepth = 1.0f;

	// Shadow Map Texture 생성 
	D3D11_TEXTURE2D_DESC texDesc = {};
	texDesc.Width = (UINT)m_ShadowViewport.Width; // 해상도 8192*8192 추천 
	texDesc.Height = (UINT)m_ShadowViewport.Height;
	texDesc.MipLevels = 1;
	texDesc.ArraySize = 1;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE; // 깊이값 기록 용도, 셰이더에서 텍스처 슬롯에 설정 활용도
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	HR_T(m_D3DDevice.GetDevice()->CreateTexture2D(&texDesc, nullptr, m_pShadowMap.GetAddressOf()));

	// DSV(depth stencil view) 생성
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	HR_T(m_D3DDevice.GetDevice()->CreateDepthStencilView(m_pShadowMap.Get(), &dsvDesc, m_pShadowMapDSV.GetAddressOf()));

	// SRV(shader resource view) 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	HR_T(m_D3DDevice.GetDevice()->CreateShaderResourceView(m_pShadowMap.Get(), &srvDesc, m_pShadowMapSRV.GetAddressOf()));

	// Comparison Sampler 생성
	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	HR_T(m_D3DDevice.GetDevice()->CreateSamplerState(&sampDesc, m_pSamplerComparison.GetAddressOf()));

	// Shadow 상수버퍼 생성
	D3D11_BUFFER_DESC bdShadow = {};
	bdShadow.Usage = D3D11_USAGE_DEFAULT;
	bdShadow.ByteWidth = sizeof(ShadowConstantBuffer);
	bdShadow.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bdShadow.CPUAccessFlags = 0;
	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&bdShadow, nullptr, m_pShadowCB.GetAddressOf()));
	
	// 기본 Sampler 생성
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR_T(m_D3DDevice.GetDevice()->CreateSamplerState(&sampDesc, m_pSamplerLinear.GetAddressOf()));


	// ---------------------------------------------------------------
	// Sky Box
	// ---------------------------------------------------------------
	D3D11_DEPTH_STENCIL_DESC dsDescSky = {};
	dsDescSky.DepthEnable = TRUE;
	dsDescSky.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // 깊이 기록 안 함
	dsDescSky.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;      // <= 비교 (끝까지 잘 보이게)
	dsDescSky.StencilEnable = FALSE;
	HR_T(m_D3DDevice.GetDevice()->CreateDepthStencilState(&dsDescSky, m_pDSState_Sky.GetAddressOf()));

	D3D11_RASTERIZER_DESC rsDesc = {};
	rsDesc.FillMode = D3D11_FILL_SOLID;        // 일반 면 채우기
	rsDesc.CullMode = D3D11_CULL_FRONT;        // 앞면을 제거, 안쪽 면이 보이도록
	rsDesc.FrontCounterClockwise = false;      // 기본 CCW 기준
	rsDesc.DepthClipEnable = true;
	HR_T(m_D3DDevice.GetDevice()->CreateRasterizerState(&rsDesc, m_pRasterizerState_Sky.ReleaseAndGetAddressOf()));


	// ---------------------------------------------------------------
	// IBL 
	// ---------------------------------------------------------------	

	// 샘플러 (s2)
	D3D11_SAMPLER_DESC sampIBL = {};
	sampIBL.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; // 선현보간으로 수행 
	sampIBL.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampIBL.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampIBL.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampIBL.MinLOD = 0.0f;							  // 최소/최대 MIP 레벨 지정
	sampIBL.MaxLOD = D3D11_FLOAT32_MAX;
	sampIBL.MipLODBias = 0.0f;
	HR_T(m_D3DDevice.GetDevice()->CreateSamplerState(&sampIBL, m_pSamplerIBL.GetAddressOf()));

	// IBL CLAMP 샘플러 (s3)
	D3D11_SAMPLER_DESC sampClamp = {};
	sampClamp.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampClamp.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampClamp.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampClamp.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampClamp.MinLOD = 0.0f;
	sampClamp.MaxLOD = D3D11_FLOAT32_MAX;
	sampClamp.MipLODBias = 0.0f;
	HR_T(m_D3DDevice.GetDevice()->CreateSamplerState(&sampClamp, m_pSamplerIBL_Clamp.GetAddressOf()));



	// ---------------------------------------------------------------
	// HDR 
	// ---------------------------------------------------------------
	
	D3D11_TEXTURE2D_DESC hdrDesc = {};
	hdrDesc.Width = (UINT)m_ClientWidth;
	hdrDesc.Height = (UINT)m_ClientHeight;
	hdrDesc.MipLevels = 1;
	hdrDesc.ArraySize = 1;
	hdrDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT; // HDR 포맷
	hdrDesc.SampleDesc.Count = 1;
	hdrDesc.Usage = D3D11_USAGE_DEFAULT;
	hdrDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	HR_T(m_D3DDevice.GetDevice()->CreateTexture2D(&hdrDesc, nullptr, m_HDRSceneTex.GetAddressOf()));

	HR_T(m_D3DDevice.GetDevice()->CreateRenderTargetView(m_HDRSceneTex.Get(), nullptr, m_HDRSceneRTV.GetAddressOf()));

	HR_T(m_D3DDevice.GetDevice()->CreateShaderResourceView(m_HDRSceneTex.Get(), nullptr, m_HDRSceneSRV.GetAddressOf()));

	// 상수버퍼 (톤맵핑용)
	D3D11_BUFFER_DESC desc{};
	desc.ByteWidth = sizeof(ToneMapConstantBuffer);
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&desc, nullptr, m_ToneMapCB.GetAddressOf()));



	// ---------------------------------------------------------------
	// 행렬(World, View, Projection) 설정
	// ---------------------------------------------------------------

	m_World = XMMatrixIdentity(); // 단위 행렬 

	m_Camera.m_Position = Vector3(-600.0f, 150.0f, -100.0f);
	m_Camera.m_Rotation = Vector3(0.0f, XMConvertToRadians(90.0f), 0.0f);

	m_Camera.SetSpeed(400.0f);

	// 카메라(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// 카메라 위치
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라가 바라보는 위치
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라의 위쪽 방향 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// 왼손 좌표계(LH) 기준 

	// 투영행렬(Projection) : 카메라의 시야각(FOV), 화면 종횡비, Near, Far 
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, m_ClientWidth / (FLOAT)m_ClientHeight, m_CameraNear, m_CameraFar);


	return true;
}

bool TestApp::InitSkyBox()
{
	// ---------------------------------------------
	// 1. 스카이박스 정점/인덱스 버퍼 생성
	// ---------------------------------------------
	const float size = 1.0f;

	// - Vector3 Pos만 가지고 있음 (CubeMap 샘플링에 필요)
	// - 총 24개의 정점, 큐브의 6면(4정점/면) 구성
	Vertex_Sky skyboxVertices[] =
	{
		{ Vector3(-size, -size, -size) }, { Vector3(-size, +size, -size) },
		{ Vector3(+size, +size, -size) }, { Vector3(+size, -size, -size) },

		{ Vector3(-size, -size, +size) }, { Vector3(+size, -size, +size) },
		{ Vector3(+size, +size, +size) }, { Vector3(-size, +size, +size) },

		{ Vector3(-size, +size, -size) }, { Vector3(-size, +size, +size) },
		{ Vector3(+size, +size, +size) }, { Vector3(+size, +size, -size) },

		{ Vector3(-size, -size, -size) }, { Vector3(+size, -size, -size) },
		{ Vector3(+size, -size, +size) }, { Vector3(-size, -size, +size) },

		{ Vector3(-size, -size, +size) }, { Vector3(-size, +size, +size) },
		{ Vector3(-size, +size, -size) }, { Vector3(-size, -size, -size) },

		{ Vector3(+size, -size, -size) }, { Vector3(+size, +size, -size) },
		{ Vector3(+size, +size, +size) }, { Vector3(+size, -size, +size) }
	};

	// Vertex Buffer
	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(Vertex_Sky) * ARRAYSIZE(skyboxVertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = skyboxVertices;

	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&vbDesc, &vbData, m_pVertexBuffer_Sky.GetAddressOf()));
	m_VertextBufferStride_Sky = sizeof(Vertex_Sky);
	m_VertextBufferOffset_Sky = 0;

	// Index Buffer : (총 12개의 삼각형, 6면 * 2삼각형/면)
	WORD skyboxIndices[] =
	{
		0,1,2, 0,2,3,
		4,5,6, 4,6,7,
		8,9,10, 8,10,11,
		12,13,14, 12,14,15,
		16,17,18, 16,18,19,
		20,21,22, 20,22,23
	};
	m_nIndices_Sky = ARRAYSIZE(skyboxIndices);	// 인덱스 개수 저장

	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(skyboxIndices);
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.Usage = D3D11_USAGE_DEFAULT;

	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = skyboxIndices;

	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&ibDesc, &ibData, m_pIndexBuffer_Sky.GetAddressOf()));



	// ---------------------------------------------
	// 2. Vertex Shader -> InputLayout ->  Pixel Shader 생성
	// ---------------------------------------------
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	// Vertex Shader
	ComPtr<ID3DBlob> vertexShaderBuffer_Sky;
	HR_T(CompileShaderFromFile(L"../Shaders/16.SkyBoxVertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer_Sky.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(vertexShaderBuffer_Sky->GetBufferPointer(), vertexShaderBuffer_Sky->GetBufferSize(), NULL, m_pVertexShader_Sky.GetAddressOf()));

	// Input Layout
	HR_T(m_D3DDevice.GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer_Sky->GetBufferPointer(), vertexShaderBuffer_Sky->GetBufferSize(), m_pInputLayout_Sky.GetAddressOf()));

	// Pixel Shader
	ComPtr<ID3DBlob> pixelShaderBuffer_Sky;
	HR_T(CompileShaderFromFile(L"../Shaders/16.SkyBoxPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer_Sky.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer_Sky->GetBufferPointer(), pixelShaderBuffer_Sky->GetBufferSize(), NULL, m_pPixelShader_Sky.GetAddressOf()));


	return true;
}


// ★ [ ImGui ] - UI 프레임 준비 및 렌더링
void TestApp::Render_ImGui()
{
	// ImGui의 입력/출력 구조체를 가져온다.  (void)io; -> 사용하지 않을 때 경고 제거용
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // 키보드로 UI 가능 
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // 게임패드로 UI 가능 


	// [ 새로운 ImGui 프레임 시작 ]
	ImGui_ImplDX11_NewFrame();		// DirectX11 렌더러용 프레임 준비
	ImGui_ImplWin32_NewFrame();		// Win32 플랫폼용 프레임 준비
	ImGui::NewFrame();				// ImGui 내부 프레임 초기화


	// 초기 창 크기 지정
	ImGui::SetNextWindowSize(ImVec2(400, 610), ImGuiCond_Always); // ImGuiCond_FirstUseEver

	// [ Control UI ]
	ImGui::Begin("Controllor");

	// UI용 폰트 적용
	ImGui::PushFont(m_UIFont);

	// -----------------------------
	// [ Light ]
	// -----------------------------
	ImGui::Text("");
	ImGui::Text("[ Light ]"); 

	// 광원 색상
	ImGui::ColorEdit3("Light Color", (float*)&m_LightColor);

	// 광원 방향 
	ImGui::DragFloat3("Light Dir", (float*)&m_LightDir, 0.01f, -1.0f, 1.0f);

	ImGui::Separator();
	ImGui::Text("");


	// -----------------------------
	// [ Camera ]
	// -----------------------------
	ImGui::Text("[ Camera ]");
	//ImGui::Text("Position (XYZ)");
	//if (ImGui::DragFloat3("Position", &m_Camera.m_Position.x, 0.5f))
	//{
	//	m_Camera.m_World = Matrix::CreateFromYawPitchRoll(m_Camera.m_Rotation) * Matrix::CreateTranslation(m_Camera.m_Position);
	//}

	//// 카메라 회전 (도 단위)
	//Vector3 rotationDegree =
	//{
	//	XMConvertToDegrees(m_Camera.m_Rotation.x),
	//	XMConvertToDegrees(m_Camera.m_Rotation.y),
	//	XMConvertToDegrees(m_Camera.m_Rotation.z)
	//};

	//ImGui::Text("Rotation (Degrees)");
	//if (ImGui::DragFloat3("Rotation", &rotationDegree.x, 0.5f))
	//{
	//	// 입력값을 라디안으로 변환하여 카메라에 적용
	//	m_Camera.m_Rotation.x = XMConvertToRadians(rotationDegree.x);
	//	m_Camera.m_Rotation.y = XMConvertToRadians(rotationDegree.y);
	//	m_Camera.m_Rotation.z = XMConvertToRadians(rotationDegree.z);

	//	// 회전 변경 시 World 행렬 갱신
	//	m_Camera.m_World = Matrix::CreateFromYawPitchRoll(m_Camera.m_Rotation) * Matrix::CreateTranslation(m_Camera.m_Position);
	//}
	// 이동 속도 조절
	ImGui::SliderFloat("Move Speed", &m_Camera.m_MoveSpeed, 10.0f, 1000.0f, "%.1f");

	ImGui::Separator();
	ImGui::Text("");

	// -----------------------------
	// [ Character Transform Control ]
	// -----------------------------

	//ImGui::Text("[ Character Transform ]");

	//// 캐릭터가 하나라도 있을 때만 표시
	//if (!m_Chars.empty())
	//{
	//	auto& chr = m_Chars[0]; // 첫 번째 캐릭터만 제어

	//	// --- Position ---
	//	ImGui::Text("Position");
	//	ImGui::DragFloat3("Char Pos", &chr->transform.position.x, 0.1f);

	//	// --- Rotation (degree) ---
	//	Vector3 rotDeg =
	//	{
	//		XMConvertToDegrees(chr->transform.rotation.x),
	//		XMConvertToDegrees(chr->transform.rotation.y),
	//		XMConvertToDegrees(chr->transform.rotation.z)
	//	};

	//	ImGui::Text("Rotation (deg)");
	//	if (ImGui::DragFloat3("Char Rot", &rotDeg.x, 0.5f))
	//	{
	//		chr->transform.rotation.x = XMConvertToRadians(rotDeg.x);
	//		chr->transform.rotation.y = XMConvertToRadians(rotDeg.y);
	//		chr->transform.rotation.z = XMConvertToRadians(rotDeg.z);
	//	}

	//	// --- Scale ---
	//	ImGui::Text("Scale");
	//	ImGui::DragFloat3("Char Scale", &chr->transform.scale.x, 0.05f, 0.01f, 100.0f);

	//	ImGui::Separator();
	//	ImGui::Text("");
	//}
	
	// -----------------------------
	// [ HDR ]
	// -----------------------------
	ImGui::Text("[ Display Mode ]");
	m_isHDRSupported ? ImGui::Text(" HDR Support") : ImGui::Text(" No HDR Support");
	if (m_SwapChainFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
		ImGui::Text(" Current Format: R10G10B10A2_UNORM (HDR ToneMapping)");
	else if (m_SwapChainFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
		ImGui::Text(" Current Format: R8G8B8A8_UNORM (LDR ToneMapping)");
	else
		ImGui::Text(" Current Format: unknown");

	ImGui::Separator();
	ImGui::Text("");

	// -----------------------------
	// [ IBL Environment ]
	// -----------------------------
	ImGui::Text("[ IBL Environment ]");
	ImGui::RadioButton("Sky", &currentIBL, 0);
	ImGui::RadioButton("Indoor", &currentIBL, 1);
	ImGui::RadioButton("Outdoor", &currentIBL, 2);
	ImGui::Separator();
	ImGui::Text("");

	// -----------------------------
	// [ IBL Control ]
	// -----------------------------
	ImGui::Text("[ IBL ]");
	ImGui::Checkbox("Use IBL", &useIBL);
	ImGui::Separator();
	ImGui::Text("");

	// -----------------------------
	// [ PBR Control ]
	// -----------------------------
	ImGui::Text("[ PBR Material ]");

	ImGui::Checkbox("Use BaseColor Texture", &useTex_Base);
	ImGui::Checkbox("Use Metallic Texture", &useTex_Metal);
	ImGui::Checkbox("Use Roughness Texture", &useTex_Rough);
	ImGui::Checkbox("Use Normal Texture", &useTex_Normal);

	if (!useTex_Base) ImGui::ColorEdit3("Manual BaseColor", (float*)&manualBaseColor);
	if (!useTex_Metal) ImGui::SliderFloat("Manual Metallic", &manualMetallic, 0.0f, 1.0f);
	if (!useTex_Rough) ImGui::SliderFloat("Manual Roughness", &manualRoughness, 0.0f, 1.0f);

	// [ 끝 ] 
	ImGui::PopFont();
	ImGui::End();


	// -----------------------------
	// 리소스 정보 출력
	// -----------------------------

	ImGui::SetNextWindowBgAlpha(0.0f); // 배경 투명
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always, ImVec2(0.0f, 0.0f)); // 왼쪽 상단 

	ImGui::Begin("DebugText", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar);

	// 글자색 검은색
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 0, 0, 255)); // RGBA

	// 디버그 전용 폰트 적용 (UI 폰트와 분리됨)
	ImGui::PushFont(m_DebugFont);

	// [ FPS 출력 ]
	ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);


	// [ 사용량 출력 ]
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);

	double totalPhysGB = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
	double usedPhysGB = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);

	double totalPageGB = memInfo.ullTotalPageFile / (1024.0 * 1024.0 * 1024.0);
	double usedPageGB = (memInfo.ullTotalPageFile - memInfo.ullAvailPageFile) / (1024.0 * 1024.0 * 1024.0);

	// GPU VRAM 조회
	IDXGIDevice* dxgiDevice = nullptr;
	IDXGIAdapter* adapter = nullptr;
	DXGI_ADAPTER_DESC desc{};
	m_D3DDevice.GetDevice()->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	if (dxgiDevice)
	{
		dxgiDevice->GetAdapter(&adapter);
		if (adapter)
		{
			adapter->GetDesc(&desc);
			adapter->Release();
		}
		dxgiDevice->Release();
	}
	double totalVRAMGB = desc.DedicatedVideoMemory / (1024.0 * 1024.0 * 1024.0);

	ImGui::Text("System RAM: %.3f / %.3f GB", usedPhysGB, totalPhysGB);
	ImGui::Text("Page File: %.3f / %.3f GB", usedPageGB, totalPageGB);
	ImGui::Text("GPU VRAM: %.3f GB", totalVRAMGB);


	// 글자색과 폰트 복원
	ImGui::PopFont();
	ImGui::PopStyleColor();

	ImGui::End();


	// [ ImGui 프레임 최종 렌더링 ]
	ImGui::Render();  // ImGui 내부에서 렌더링 데이터를 준비
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // DX11로 실제 화면에 출력
}

void TestApp::Render_DebugDraw()
{
	// [ Shadow Frustum ]
	if (m_DrawShadowFrustum)
	{
		auto* context = m_D3DDevice.GetDeviceContext();

		// -----------------------------
		// DebugDraw 전용 상태 설정
		// -----------------------------
		context->IASetInputLayout(m_DebugInputLayout.Get());
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

		m_DebugEffect->SetView(m_View);           // 메인 카메라
		m_DebugEffect->SetProjection(m_Projection);
		m_DebugEffect->Apply(context);

		m_DebugBatch->Begin();

		// Shadow Frustum
		m_DebugDraw->Draw(m_DebugBatch.get(), m_ShadowFrustumWS, DirectX::Colors::Red);

		// Shadow Camera 위치
		DirectX::BoundingSphere lightPos;
		lightPos.Center = {
			m_ShadowCameraPos.x,
			m_ShadowCameraPos.y,
			m_ShadowCameraPos.z
		};
		lightPos.Radius = 20.0f;

		m_DebugDraw->Draw(m_DebugBatch.get(), lightPos, DirectX::Colors::Yellow);

		m_DebugBatch->End();
	}
}

bool TestApp::CheckHDRSupportAndGetMaxNits(float& outMaxNits, DXGI_FORMAT& outFormat)
{
	ComPtr<IDXGIFactory4> pFactory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&pFactory));
	if (FAILED(hr))
	{
		LOG_ERRORA("ERROR: DXGI Factory 생성 실패.\n");
		return false;
	}
	// 2. 주 그래픽 어댑터 (0번) 열거
	ComPtr<IDXGIAdapter1> pAdapter;
	UINT adapterIndex = 0;
	while (pFactory->EnumAdapters1(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC1 desc;
		pAdapter->GetDesc1(&desc);

		// WARP 어댑터(소프트웨어)를 건너뛰고 주 어댑터만 사용하도록 선택할 수 있습니다.
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapterIndex++;
			pAdapter.Reset();
			continue;
		}
		break;
	}

	if (!pAdapter)
	{
		LOG_ERRORA("ERROR: 유효한 하드웨어 어댑터를 찾을 수 없습니다.\n");
		return false;
	}

	// 3. 주 모니터 출력 (0번) 열거
	ComPtr<IDXGIOutput> pOutput;
	hr = pAdapter->EnumOutputs(0, &pOutput); // 0번 출력
	if (FAILED(hr))
	{
		LOG_ERRORA("ERROR: 주 모니터 출력(Output 0)을 찾을 수 없습니다.\n");
		return false;
	}

	// 4. HDR 정보를 얻기 위해 IDXGIOutput6으로 쿼리
	ComPtr<IDXGIOutput6> pOutput6;
	hr = pOutput.As(&pOutput6);
	if (FAILED(hr))
	{
		printf("INFO: IDXGIOutput6 인터페이스를 얻을 수 없습니다. HDR 정보를 얻을 수 없습니다.\n");
		outMaxNits = 100.0f;
		outFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		return false;
	}

	// 5. DXGI_OUTPUT_DESC1에서 HDR 정보 확인
	DXGI_OUTPUT_DESC1 desc1 = {};
	hr = pOutput6->GetDesc1(&desc1);
	if (FAILED(hr))
	{
		printf("ERROR: GetDesc1 호출 실패.\n");
		return false;
	}

	// 6. HDR 활성화 조건 분석
	bool isHDRColorSpace = (desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
	outMaxNits = (float)desc1.MaxLuminance;

	// OS가 HDR을 켰을 때 MaxLuminance는 100 Nits(SDR 기준)를 초과합니다.
	bool isHDRActive = outMaxNits > 100.0f;

	if (isHDRColorSpace && isHDRActive)
	{
		// 최종 판단: HDR 지원 및 OS 활성화
		outFormat = DXGI_FORMAT_R10G10B10A2_UNORM; // HDR 포맷 설정
		printf("SUCCESS: HDR 활성화됨. MaxNits: %.1f, Format: R10G10B10A2_UNORM\n", outMaxNits);
		return true;
	}
	else
	{
		// HDR 지원 안함 또는 OS에서 비활성화
		outMaxNits = 100.0f; // SDR 기본값
		outFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // SDR 포맷 설정
		printf("INFO: HDR 비활성화. MaxNits: 100.0, Format: R8G8B8A8_UNORM\n");
		return false;
	}
	return true;
}

void TestApp::UninitScene()
{
	OutputDebugString(L"[TestApp::UninitScene] 실행\r\n");

	// 샘플러, 상수버퍼, 셰이더, 입력레이아웃 해제
	m_pSamplerLinear.Reset();
	m_pSamplerComparison.Reset();
	m_pConstantBuffer.Reset();
	m_pVertexShader.Reset();
	m_pPixelShader.Reset();
	m_pInputLayout.Reset();
	m_pShadowVS.Reset();
	m_pShadowPS.Reset();
	m_pShadowInputLayout.Reset();
	m_pShadowCB.Reset();
	m_pShadowMap.Reset();
	m_pShadowMapDSV.Reset();
	m_pShadowMapSRV.Reset();
	m_pVertexShader_Sky.Reset();
	m_pPixelShader_Sky.Reset();
	m_pInputLayout_Sky.Reset();
	m_pVertexBuffer_Sky.Reset();
	m_pIndexBuffer_Sky.Reset();
	m_pDSState_Sky.Reset();
	m_pRasterizerState_Sky.Reset();
	m_pIrradianceSRV.Reset();
	m_pPrefilterSRV.Reset();
	m_pBRDFLUTSRV.Reset();
	m_pSamplerIBL.Reset();
	m_pSamplerIBL_Clamp.Reset();

	m_ToneMapCB.Reset();
	m_pToneMapVS.Reset();
	m_pToneMapPS_HDR.Reset();
	m_pToneMapPS_LDR.Reset();
	m_HDRSceneTex.Reset();
	m_HDRSceneRTV.Reset();
	m_HDRSceneSRV.Reset();

	// 인스턴스 해제
	m_Humans.clear();
	m_Planes.clear();
	m_Chars.clear();
	m_Vampires.clear();
	m_IBLSet.clear();

	humanAsset.reset();
	charAsset.reset();
	planeAsset.reset();
	VampireAsset.reset();

	m_DebugBatch.reset();
	m_DebugEffect.reset();
	m_DebugDraw.reset();
	m_DebugInputLayout.Reset();


	AssetManager::Get().UnloadAll();
	Material::DestroyDefaultTextures(); // 기본 텍스처 
}


// [ ImGui ]
bool TestApp::InitImGUI()
{
	// 1. ImGui 버전 확인 
	IMGUI_CHECKVERSION();   

	// 2. ImGui 컨텍스트 생성
	ImGui::CreateContext(); 

	// 3. ImGui 스타일 설정
	ImGui::StyleColorsDark(); // ImGui::StyleColorsLight();

	ImGuiIO& io = ImGui::GetIO();

	// 1. UI 폰트 (Arial)
	m_UIFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\arial.ttf", 13.0f);

	// 2. DebugText 폰트 (Consolas)
	m_DebugFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 15.0f);

	// 4. 플랫폼 및 렌더러 백엔드 초기화
	ImGui_ImplWin32_Init(m_hWnd);												// Win32 플랫폼용 초기화 (윈도우 핸들 필요)
	ImGui_ImplDX11_Init(this->m_D3DDevice.GetDevice(), this->m_D3DDevice.GetDeviceContext());	// DirectX11 렌더러용 초기화


	return true;
}

void TestApp::UninitImGUI()
{
	ImGui_ImplDX11_Shutdown();	// DX11용 ImGui 렌더러 정리
	ImGui_ImplWin32_Shutdown(); // Win32 플랫폼용 ImGui 정리
	ImGui::DestroyContext();	// ImGui 컨텍스트 삭제
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK TestApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return __super::WndProc(hWnd, message, wParam, lParam);
}