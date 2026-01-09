#include "TestApp.h"
#include "../Common/AssetManager.h"
#include "../Common/Helper.h"

#include <d3dcompiler.h>
#include <Directxtk/DDSTextureLoader.h>
#include <windows.h>
#include <dxgi1_6.h>
#include "Dance1State.h"
#include "Dance2State.h"
#include "Human_3_State.h"

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
	// Physics 초기화
	// ------------------------------------
	 auto& physics = *PhysicsSystem::Get().GetPhysics();
	 auto& scene = *PhysicsSystem::Get().GetScene();

	// planeAsset (바닥)
	m_Plane = PhysicsHelper::CreateStaticBox(PxVec3(0, -1, 0), PxVec3(50, 1, 50));

	// charAsset (static)
	m_CharAsset = PhysicsHelper::CreateStaticBox(PxVec3(-5, 0, 5), PxVec3(1, 2, 1));

	// treeAsset (static)
	m_TreeAsset = PhysicsHelper::CreateStaticBox(PxVec3(5, 0, 5), PxVec3(1, 3, 1));

	// Human_1 (static 캐릭터)
	m_Human1 = PhysicsHelper::CreateStaticBox(PxVec3(-3, 0, -3), PxVec3(0.5f, 1.8f, 0.5f));

	// Human_2 (Dynamic)
	m_Human2 = PhysicsHelper::CreateDynamicBox(PxVec3(0, 5, -3), PxVec3(0.5f, 1.8f, 0.5f), 10.0f);


	// Human_3 (Character Controller)
	m_Human3 = PhysicsSystem::Get().CreateCapsuleController(
		PxExtendedVec3(3, 2, -3),
		0.4f,
		1.6f,
		10.0f
	);

	if (!m_Human3)
	{
		LOG_ERRORA("CreateCapsuleController failed!");
		return false;
	}


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
	UninitScene();      
	UninitImGUI();
	m_D3DDevice.Cleanup();
	CheckDXGIDebug();	// DirectX 리소스 누수 체크
}


// [ 리소스 로드 (Asset) ] 
bool TestApp::LoadAsset()
{
	auto* device = m_D3DDevice.GetDevice();

	auto CreateStatic = [&](std::shared_ptr<StaticMeshAsset> asset, Vector3 pos, Vector3 rot = { 0,0,0 }, Vector3 scale = { 1,1,1 })
		{
			auto inst = std::make_shared<StaticMeshInstance>();
			inst->SetAsset(asset);
			inst->transform.position = pos;
			inst->transform.rotation = rot;
			inst->transform.scale = scale;
			m_StaticMeshes.push_back(inst);
		};

	auto CreateSkeletal = [&](std::shared_ptr<SkeletalMeshAsset> asset, Vector3 pos, Vector3 rot = { 0,0,0 }, Vector3 scale = { 1,1,1 },
		const std::string& name = "NoneName")
		{
			auto inst = std::make_shared<SkeletalMeshInstance>();
			inst->m_Name = name;
			inst->SetAsset(device, asset);
			inst->transform.position = pos;
			inst->transform.rotation = rot;
			inst->transform.scale = scale;
			m_SkeletalMeshes.push_back(inst);

			if(name == "Human_2")
			{
				const AnimationClip* dance1 = asset->GetAnimation("Dance_1");
				const AnimationClip* dance2 = asset->GetAnimation("Dance_2");

				// 상태 등록
				inst->m_Controller.AddState(std::make_unique<Dance1State>(dance1, &inst->m_Controller));
				inst->m_Controller.AddState(std::make_unique<Dance2State>(dance2, &inst->m_Controller));

				// 초기 상태
				inst->m_Controller.ChangeState("Dance_1", 0.0f);
			}

			if (name == "Human_3")
			{
				const AnimationClip* idle = asset->GetAnimation("Idle");
				const AnimationClip* attack = asset->GetAnimation("Attack");
				const AnimationClip* die = asset->GetAnimation("Die");


				// 상태 등록
				inst->m_Controller.AddState(std::make_unique<IdleState>(idle, &inst->m_Controller));
				inst->m_Controller.AddState(std::make_unique<AttackState>(attack, &inst->m_Controller));
				inst->m_Controller.AddState(std::make_unique<DieState>(die, &inst->m_Controller));

				// FSM 파라미터 초기화
				auto& params = inst->m_Controller.Params;
				params.SetBool("Idle", true);
				params.SetBool("Attack", false);
				params.SetBool("Die", false);

				// 초기 상태
				inst->m_Controller.ChangeState("Idle_State", 0.0f);
			}
		};


	// ---------------------------------------------
	// Static Mesh
	// ---------------------------------------------
	planeAsset = AssetManager::Get().LoadStaticMesh(device, "../Resource/Plane.fbx");
	charAsset = AssetManager::Get().LoadStaticMesh(device, "../Resource/char/char.fbx");
	treeAsset = AssetManager::Get().LoadStaticMesh(device, "../Resource/Tree.fbx");

	CreateStatic(planeAsset, { 100, -15, 0 }, { 0, 0, 0 }, { 0.5f, 1.0f, 0.5f });
	CreateStatic(charAsset, { 0, 0, -90 }, { 0, XMConvertToRadians(90), 0 });
	CreateStatic(treeAsset, { 200, 0, 100 }, { 0, XMConvertToRadians(90), 0 });
	CreateStatic(treeAsset, { 200, 0, -130 }, { 0, XMConvertToRadians(90), 0 });



	// ---------------------------------------------
	// Skeletal Mesh 
	// ---------------------------------------------
	humanAsset = AssetManager::Get().LoadSkeletalMesh(device, "../Resource/Skeletal/DancingHuman.fbx");
	CharacterAsset = AssetManager::Get().LoadSkeletalMesh(device, "../Resource/Skeletal/Character.fbx");

	// [ 애니메이션 로드 ]
	humanAsset->LoadAnimationFromFBX("../Resource/Animation/Human_1_Dance1.fbx", "Dance_1");
	humanAsset->LoadAnimationFromFBX("../Resource/Animation/Human_1_Dance2.fbx", "Dance_2");
	CharacterAsset->LoadAnimationFromFBX("../Resource/Animation/Human_3_Idle.fbx", "Idle");
	CharacterAsset->LoadAnimationFromFBX("../Resource/Animation/Human_3_Attack.fbx", "Attack", false);
	CharacterAsset->LoadAnimationFromFBX("../Resource/Animation/Human_3_Die.fbx", "Die", false);

	CreateSkeletal(humanAsset, { -40, 0, 100 }, { 0, XMConvertToRadians(45), 0 }, { 1,1,1 }, "Human_1");
	CreateSkeletal(humanAsset, { -10, 0, 30 }, { 0, XMConvertToRadians(45), 0 }, { 1,1,1 }, "Human_2");
	CreateSkeletal(CharacterAsset, { 50, 0, -130 }, { 0, XMConvertToRadians(45), 0 }, { 1,1,1 }, "Human_3");

	


	// ---------------------------------------------
	// SkyBox 리소스 
	// ---------------------------------------------
	m_IBLSet.resize(3);

	// Sky 
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/Sky/Sky_EnvHDR.dds", m_IBLSet[0].skyboxSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/Sky/Sky_DiffuseHDR.dds", m_IBLSet[0].irradianceSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/Sky/Sky_SpecularHDR.dds", m_IBLSet[0].prefilterSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/Sky/Sky_Brdf.dds", m_IBLSet[0].brdfLutSRV.ReleaseAndGetAddressOf()));

	// InDoor 
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/cubemapEnvHDR.dds", m_IBLSet[1].skyboxSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/cubemapDiffuseHDR.dds", m_IBLSet[1].irradianceSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/cubemapSpecularHDR.dds", m_IBLSet[1].prefilterSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/cubemapBrdf.dds", m_IBLSet[1].brdfLutSRV.ReleaseAndGetAddressOf()));

	// OutDoor 
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/_OutDoor/OutDoor_EnvHDR.dds", m_IBLSet[2].skyboxSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/_OutDoor/OutDoor_DiffuseHDR.dds", m_IBLSet[2].irradianceSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/_OutDoor/OutDoor_SpecularHDR.dds", m_IBLSet[2].prefilterSRV.ReleaseAndGetAddressOf()));
	HR_T(CreateTextureFromFile(device, L"../Resource/SkyBox/_OutDoor/OutDoor_Brdf.dds", m_IBLSet[2].brdfLutSRV.ReleaseAndGetAddressOf()));

	return true; 
}


void TestApp::Update()
{
	__super::Update();

	float deltaTime = TimeSystem::m_Instance->DeltaTime();
	float totalTime = TimeSystem::m_Instance->TotalTime();

	m_Camera.GetViewMatrix(m_View);			// View 행렬 갱신


	// Skeletal Update (업데이트된 world 전달)
	for (auto& mesh : m_SkeletalMeshes)
	{
		mesh->Update(deltaTime);
	}
	for (auto& mesh : m_StaticMeshes)
	{
		mesh->Update();
	}


	// [ 임시 이동 ] 
	PxVec3 move(0);

	if (GetAsyncKeyState(VK_UP) & 0x8000) move.z -= 1;
	if (GetAsyncKeyState(VK_DOWN) & 0x8000) move.z += 1;	
	if (GetAsyncKeyState(VK_LEFT) & 0x8000) move.x -= 1;	
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000) move.x += 1;
	if (move.magnitudeSquared() > 0) move.normalize();
	
	move *= m_MoveSpeed * deltaTime;

	PxControllerFilters filters;
	m_Human3->move(move, 0.01f, deltaTime, filters);

	
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
	cb.vLightColor = XMFLOAT4(m_LightColor.x,m_LightColor.y,m_LightColor.z,m_LightIntensity); // 강도는 w
	cb.vEyePos = XMFLOAT4(m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z, 1.0f);
	cb.gIsRigid = isRigid;

	cb.gMetallicMultiplier = useTex_Metal ? XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f) : XMFLOAT4(manualMetallic, 0.0f, 0.0f, 0.0f); // 텍스처 사용 여부에 따라 GUI 값 적용
	cb.gRoughnessMultiplier = useTex_Rough ? XMFLOAT4(1.0f, 0.0f, 0.0f, 0.0f) : XMFLOAT4(manualRoughness, 0.0f, 0.0f, 0.0f);
	cb.manualBaseColor = manualBaseColor; 

	cb.useTexture_BaseColor = useTex_Base;
	cb.useTexture_Metallic = useTex_Metal;
	cb.useTexture_Roughness = useTex_Rough;
	cb.useTexture_Normal = useTex_Normal;
	cb.useIBL = useIBL; 

	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_D3DDevice.GetDeviceContext()->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
}


// --------------------------------------------
//	렌더링 파이프라인 (Deferred Shading)
// --------------------------------------------
//	[1] ShadowMap Pass
//  [2] G-Buffer Pass	
//  [3] Deferred Lighting Pass	(G-Buffer → HDR)
//	[4] ToneMapping Pass		(HDR → LDR)
//	[5] UI / Debug Pass
//	[6] Present

void TestApp::Render()
{
	// --------------------------------------------
	// [1] DepthOnly Pass 렌더링 (ShadowMap) 
	// --------------------------------------------
	Render_ShadowMap(); 


	// --------------------------------------------
	// [2] G-Buffer Pass
	// --------------------------------------------
	// 월드 위치, 법선, 알베도, MR, Emissive 등 G-Buffer에 저장  
	Render_BeginGBuffer();      
	Render_GBufferGeometry();   


	// --------------------------------------------
	// [3] Deferred Lighting Pass
	// --------------------------------------------
	// G-Buffer → Fullscreen Quad → Lighting 계산(PBR + Shadow + IBL)
	Render_BeginSceneHDR();		// HDR RT 바인딩 
	Render_SkyBox();			// SkyBox 렌더링 (Depth Write OFF)
	Render_DeferredLighting();	// Fullscreen Quad (PBR + Light + Shadow + IBL)

	
	// --------------------------------------------
	// [4] Tone Mapping Pass
	// --------------------------------------------
	Render_BeginBackBuffer(); // BackBuffer RTV
	Render_ToneMapping();     // Fullscreen Quad


	// --------------------------------------------
	// [5] UI / Debug (LDR)
	// --------------------------------------------
	Render_ImGui();		// UI 렌더링
	Render_DebugDraw(); // Debug Draw 렌더링
	

	// --------------------------------------------
	// [6] Present 
	// --------------------------------------------
	m_D3DDevice.EndFrame(); 
}


// [1] Shadow Map 렌더링
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
	for (auto& mesh : m_SkeletalMeshes)
		RenderShadowSkeletal(*mesh, mesh->GetWorld());

	for (auto& mesh : m_StaticMeshes)
		RenderShadowStatic(*mesh, mesh->GetWorld());


	// RenderTarget / Viewport 복원
	context->RSSetViewports(1, &m_D3DDevice.GetViewport());
	ID3D11RenderTargetView* rtv = m_D3DDevice.GetRenderTargetView();
	context->OMSetRenderTargets(1, &rtv, m_D3DDevice.GetDepthStencilView());
}


// [2] G-Buffer
void TestApp::Render_BeginGBuffer()
{
	auto* context = m_D3DDevice.GetDeviceContext();

	// Clear
	float clearPos[4] = { 0, 0, 0, 1 };
	float clearNormal[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float clearAlbedo[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	float clearMR[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float clearEmissive[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	
	context->ClearRenderTargetView(m_GBufferRTV[0].Get(), clearPos);		// Position
	context->ClearRenderTargetView(m_GBufferRTV[1].Get(), clearNormal);		// Normal
	context->ClearRenderTargetView(m_GBufferRTV[2].Get(), clearAlbedo);		// Albedo
	context->ClearRenderTargetView(m_GBufferRTV[3].Get(), clearMR);			// MR
	context->ClearRenderTargetView(m_GBufferRTV[4].Get(), clearEmissive);	// Emissive
	context->ClearDepthStencilView(m_D3DDevice.GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	ID3D11RenderTargetView* rtvs[] =
	{
		m_GBufferRTV[0].Get(), // Position
		m_GBufferRTV[1].Get(), // Normal
		m_GBufferRTV[2].Get(), // Albedo
		m_GBufferRTV[3].Get(), // MR
		m_GBufferRTV[4].Get()  // Emissive
	};
	context->OMSetRenderTargets(_countof(rtvs), rtvs, m_D3DDevice.GetDepthStencilView()); 

	context->RSSetViewports(1, &m_Viewport);

	context->OMSetDepthStencilState(m_pDSState_GBuffer.Get(), 0);  // Depth test ON, write ON

	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	context->IASetInputLayout(m_pInputLayout.Get());

	context->VSSetShader(m_pGBufferVS.Get(), nullptr, 0); 
	context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	context->PSSetShader(m_pGBufferPS.Get(), nullptr, 0);
	context->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	context->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	// G-Buffer SRV 해제
	ID3D11ShaderResourceView* nullSRV[GBufferCount] = {};
	context->PSSetShaderResources(20, GBufferCount, nullSRV);
	context->VSSetShaderResources(20, GBufferCount, nullSRV);

}


// [2] G-Buffer
void TestApp::Render_GBufferGeometry()
{
	for (auto& mesh : m_StaticMeshes)
		RenderStaticMesh(*mesh);

	for (auto& mesh : m_SkeletalMeshes)
		RenderSkeletalMesh(*mesh);
}


// [3] Deferred Lighting Pass
void TestApp::Render_BeginSceneHDR()
{
	// HDR RT + 기존 DepthStencil
	ID3D11RenderTargetView* rtvs[] = { m_HDRSceneRTV.Get() };
	m_D3DDevice.GetDeviceContext()->OMSetRenderTargets(1, rtvs, m_D3DDevice.GetDepthStencilView());

	// Clear 
	const float clearColor[4] = { 0, 0, 0, 1 };
	m_D3DDevice.GetDeviceContext()->ClearRenderTargetView(m_HDRSceneRTV.Get(), clearColor);

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


// [3] Deferred Lighting Pass
void TestApp::Render_DeferredLighting()
{
	auto* context = m_D3DDevice.GetDeviceContext();

	// Geometry 관련 상태 전부 무효화
	context->IASetInputLayout(nullptr);
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// HDR RT 바인딩, Depth 필요 없음
	ID3D11RenderTargetView* rtvs[] = { m_HDRSceneRTV.Get() };
	context->OMSetRenderTargets(1, rtvs, m_D3DDevice.GetDepthStencilView());

	// DepthStencilState도 Depth OFF로 변경
	context->OMSetDepthStencilState(m_pDSState_Deferred.Get(), 0); // Depth test OFF, write OFF

	context->RSSetViewports(1, &m_Viewport);

	context->VSSetShader(m_pToneMapVS.Get(), nullptr, 0);
	context->PSSetShader(m_pDeferredLightingPS.Get(), nullptr, 0);
	
	// Deferred Lighting PS에서도 b0, b3 필요함
	context->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(3, 1, m_pShadowCB.GetAddressOf());

	// G-Buffer SRV
	context->PSSetShaderResources(20, 1, m_GBufferSRV[0].GetAddressOf()); // Position
	context->PSSetShaderResources(21, 1, m_GBufferSRV[1].GetAddressOf()); // Normal
	context->PSSetShaderResources(22, 1, m_GBufferSRV[2].GetAddressOf()); // Albedo
	context->PSSetShaderResources(23, 1, m_GBufferSRV[3].GetAddressOf()); // MR
	context->PSSetShaderResources(24, 1, m_GBufferSRV[4].GetAddressOf()); // Emissive

	// ShadowMap
	context->PSSetShaderResources(6, 1,m_pShadowMapSRV.GetAddressOf());

	// IBL
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

	// Samplers
	context->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
	context->PSSetSamplers(1, 1, m_pSamplerComparison.GetAddressOf());
	context->PSSetSamplers(2, 1, m_pSamplerIBL.GetAddressOf());
	context->PSSetSamplers(3, 1, m_pSamplerIBL_Clamp.GetAddressOf());

	// Fullscreen Triangle
	context->Draw(3, 0);

	// SRV Unbind
	ID3D11ShaderResourceView* nullSRV[GBufferCount] = {};
	context->PSSetShaderResources(20, GBufferCount, nullSRV);
}

void TestApp::Render_SkyBox()
{
	// [ 스카이박스 렌더링 전 (깊이 테스트, 래스터라이저 상태 변경) ]
	m_D3DDevice.GetDeviceContext()->OMSetDepthStencilState(m_pDSState_Sky.Get(), 1);
	m_D3DDevice.GetDeviceContext()->RSSetState(m_pRasterizerState_Sky.Get());

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


void TestApp::Render_ToneMapping()
{
	auto* context = m_D3DDevice.GetDeviceContext();

	context->OMSetDepthStencilState(nullptr, 0);

	context->IASetInputLayout(nullptr); 
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(m_pToneMapVS.Get(), nullptr, 0);

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
	toneMapCB.Time = TimeSystem::m_Instance->TotalTime();
	toneMapCB.gEnableDistortion = m_EnableDistortion ? 1 : 0;
	context->UpdateSubresource(m_ToneMapCB.Get(), 0, nullptr, &toneMapCB, 0, 0);
	context->PSSetConstantBuffers(4, 1, m_ToneMapCB.GetAddressOf());

	// Draw fullscreen
	context->Draw(3, 0);

	// SRV Unbind
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	context->PSSetShaderResources(7, 1, nullSRV);
}


// [ Scene 초기화 ] 
bool TestApp::InitScene()
{
	HRESULT hr = 0;                       // DirectX 함수 결과값
	ID3D10Blob* errorMessage = nullptr;   // 셰이더 컴파일 에러 메시지 버퍼	

	// ---------------------------------------------------------------
	// 버텍스 셰이더(Vertex Shader) 컴파일 및 생성
	// ---------------------------------------------------------------
	ComPtr<ID3DBlob> ShadowVSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/17/17.ShadowVertexShader.hlsl", "ShadowVS", "vs_4_0", ShadowVSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(ShadowVSBuffer->GetBufferPointer(), ShadowVSBuffer->GetBufferSize(), NULL, m_pShadowVS.GetAddressOf()));

	ComPtr<ID3DBlob> ToneVSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/17/17.ToneVertexShader.hlsl", "main", "vs_4_0", ToneVSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(ToneVSBuffer->GetBufferPointer(), ToneVSBuffer->GetBufferSize(), NULL, m_pToneMapVS.GetAddressOf()));

	ComPtr<ID3DBlob> GBufferVSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/17/17.G_Buffer_VertexShader.hlsl", "main", "vs_4_0", GBufferVSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(GBufferVSBuffer->GetBufferPointer(), GBufferVSBuffer->GetBufferSize(), NULL, m_pGBufferVS.GetAddressOf()));


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
	HR_T(m_D3DDevice.GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), GBufferVSBuffer->GetBufferPointer(), GBufferVSBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));

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
	ComPtr<ID3DBlob> ShadowPSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/17/17.ShadowPixelShader.hlsl", "ShadowPS", "ps_4_0", ShadowPSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(ShadowPSBuffer->GetBufferPointer(), ShadowPSBuffer->GetBufferSize(), NULL, m_pShadowPS.GetAddressOf()));

	ComPtr<ID3DBlob> TonePSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/17/17.TonePixelShader_HDR.hlsl", "main", "ps_4_0", TonePSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(TonePSBuffer->GetBufferPointer(), TonePSBuffer->GetBufferSize(), NULL, m_pToneMapPS_HDR.GetAddressOf()));

	HR_T(CompileShaderFromFile(L"../Shaders/17/17.TonePixelShader_LDR.hlsl", "main", "ps_4_0", TonePSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(TonePSBuffer->GetBufferPointer(), TonePSBuffer->GetBufferSize(), NULL, m_pToneMapPS_LDR.GetAddressOf()));

	ComPtr<ID3DBlob> DeferredLightingPSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/17/17.Deferred_PixelShader.hlsl", "main", "ps_4_0", DeferredLightingPSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(DeferredLightingPSBuffer->GetBufferPointer(), DeferredLightingPSBuffer->GetBufferSize(), NULL, m_pDeferredLightingPS.GetAddressOf()));

	HR_T(CompileShaderFromFile(L"../Shaders/17/17.G_Buffer_PixelShader.hlsl", "main", "ps_4_0", DeferredLightingPSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(DeferredLightingPSBuffer->GetBufferPointer(), DeferredLightingPSBuffer->GetBufferSize(), NULL, m_pGBufferPS.GetAddressOf()));



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
	srvDesc.Texture2D.MostDetailedMip = 0;
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

	// DebugDraw 깊이스텐실 상태
	D3D11_DEPTH_STENCIL_DESC ds = {};
	ds.DepthEnable = FALSE;
	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	ds.DepthFunc = D3D11_COMPARISON_ALWAYS;
	ds.StencilEnable = FALSE;
	HR_T(m_D3DDevice.GetDevice()->CreateDepthStencilState(&ds, m_pDSState_DebugDraw.GetAddressOf()));


	// --------------------------------------------------------------
	// Deferred Shading G-Buffer 
	// --------------------------------------------------------------

	struct RTDesc
	{
		DXGI_FORMAT format;
	};

	RTDesc formats[GBufferCount] = {
		{ DXGI_FORMAT_R16G16B16A16_FLOAT }, // WorldPos 
		{ DXGI_FORMAT_R16G16B16A16_FLOAT }, // Normal 
		{ DXGI_FORMAT_R8G8B8A8_UNORM_SRGB },// Albedo
		{ DXGI_FORMAT_R8G8B8A8_UNORM },     // Metallic / Roughness
		{ DXGI_FORMAT_R11G11B10_FLOAT }		// Emissive 
	};

	for (int i = 0; i < GBufferCount; ++i)
	{
		D3D11_TEXTURE2D_DESC desc{};
		desc.Width = (float)m_ClientWidth;
		desc.Height = (float)m_ClientHeight;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = formats[i].format;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		HR_T(m_D3DDevice.GetDevice()->CreateTexture2D(&desc, nullptr, m_GBufferTex[i].GetAddressOf()));
		HR_T(m_D3DDevice.GetDevice()->CreateRenderTargetView(m_GBufferTex[i].Get(), nullptr, m_GBufferRTV[i].GetAddressOf()));
		HR_T(m_D3DDevice.GetDevice()->CreateShaderResourceView(m_GBufferTex[i].Get(), nullptr, m_GBufferSRV[i].GetAddressOf()));
	}

	D3D11_DEPTH_STENCIL_DESC dsDesc{};
	dsDesc.DepthEnable = FALSE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	dsDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	HR_T(m_D3DDevice.GetDevice()->CreateDepthStencilState(&dsDesc, m_pDSState_Deferred.GetAddressOf()));

	D3D11_DEPTH_STENCIL_DESC dsGBuffer{};
	dsGBuffer.DepthEnable = TRUE;							
	dsGBuffer.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;	
	dsGBuffer.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;			
	dsGBuffer.StencilEnable = FALSE;
	HR_T(m_D3DDevice.GetDevice()->CreateDepthStencilState(&dsGBuffer, m_pDSState_GBuffer.GetAddressOf()));

	// viewport
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;
	m_Viewport.Width = (float)m_ClientWidth;
	m_Viewport.Height = (float)m_ClientHeight;
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;


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
	HR_T(CompileShaderFromFile(L"../Shaders/17/17.SkyBoxVertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer_Sky.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(vertexShaderBuffer_Sky->GetBufferPointer(), vertexShaderBuffer_Sky->GetBufferSize(), NULL, m_pVertexShader_Sky.GetAddressOf()));

	// Input Layout
	HR_T(m_D3DDevice.GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer_Sky->GetBufferPointer(), vertexShaderBuffer_Sky->GetBufferSize(), m_pInputLayout_Sky.GetAddressOf()));

	// Pixel Shader
	ComPtr<ID3DBlob> pixelShaderBuffer_Sky;
	HR_T(CompileShaderFromFile(L"../Shaders/17/17.SkyBoxPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer_Sky.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer_Sky->GetBufferPointer(), pixelShaderBuffer_Sky->GetBufferSize(), NULL, m_pPixelShader_Sky.GetAddressOf()));


	return true;
}


// [ ImGui ] - UI 프레임 준비 및 렌더링
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
	ImGui::SetNextWindowSize(ImVec2(400, 900), ImGuiCond_Always); // ImGuiCond_FirstUseEver

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

	// 광원 세기 (HDR 대응)
	ImGui::DragFloat(
		"Light Intensity",
		&m_LightIntensity,
		0.1f,     // step
		0.0f,     // min
		50.0f,    // max (HDR용)
		"%.2f"
	);

	ImGui::Separator();
	ImGui::Text("");


	// -----------------------------
	// [ Camera ]
	// -----------------------------
	ImGui::Text("[ Camera ]");

	ImGui::SliderFloat("Move Speed", &m_Camera.m_MoveSpeed, 10.0f, 1000.0f, "%.1f");

	ImGui::Separator();
	ImGui::Text("");


	// -----------------------------
	// [ HDR ]
	// -----------------------------
	ImGui::Text("[ Display Mode ]");
	m_isHDRSupported ? ImGui::Text(" HDR Support") : ImGui::Text(" No HDR Support");
	if (m_SwapChainFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
		ImGui::Text(" Current Format : R10G10B10A2_UNORM (HDR ToneMapping)");
	else if (m_SwapChainFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
		ImGui::Text(" Current Format : R8G8B8A8_UNORM (LDR ToneMapping)");
	else
		ImGui::Text(" Current Format : unknown");

	ImGui::DragFloat("Exposure", &m_ExposureEV, 0.1f, -5.0f, 5.0f);

	ImGui::Separator();
	ImGui::Text("");


	// -----------------------------
	// [ ScreenEffect ]
	// -----------------------------
	ImGui::Text("Screen Effect");
	ImGui::Checkbox("Screen Distortion", &m_EnableDistortion);
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

	if (!useTex_Base) ImGui::ColorEdit3("BaseColor", (float*)&manualBaseColor);
	if (!useTex_Metal) ImGui::SliderFloat("Metallic", &manualMetallic, 0.0f, 1.0f);
	if (!useTex_Rough) ImGui::SliderFloat("Roughness", &manualRoughness, 0.0f, 1.0f);

	// [ 끝 ] 
	ImGui::PopFont();
	ImGui::End();


	// -----------------------------
	// Debug View
	// -----------------------------
	ImGui::Begin("G-Buffer Debug View");

	ImGui::BeginGroup();
	ImGui::Text("Position");
	ImGui::Image((ImTextureID)m_GBufferSRV[0].Get(), ImVec2(128, 128));
	ImGui::EndGroup();

	ImGui::SameLine();

	ImGui::BeginGroup();
	ImGui::Text("Normal");
	ImGui::Image((ImTextureID)m_GBufferSRV[1].Get(), ImVec2(128, 128));
	ImGui::EndGroup();

	ImGui::BeginGroup();
	ImGui::Text("Albedo");
	ImGui::Image((ImTextureID)m_GBufferSRV[2].Get(), ImVec2(128, 128));
	ImGui::EndGroup();

	ImGui::SameLine();

	// 메탈릭/러프니스 G-Buffer
	ImGui::BeginGroup();
	ImGui::Text("Metallic / Roughness");
	ImGui::Image((ImTextureID)m_GBufferSRV[3].Get(), ImVec2(128, 128));
	ImGui::EndGroup();

	ImGui::BeginGroup();
	ImGui::Text("Emissive");
	ImGui::Image((ImTextureID)m_GBufferSRV[4].Get(), ImVec2(128, 128));
	ImGui::EndGroup();


	// -----------------------------
	// ShadowMap
	// -----------------------------
	ImGui::BeginGroup();
	ImGui::Text("ShadowMap");
	ImGui::Image( (ImTextureID)m_pShadowMapSRV.Get(), ImVec2(256, 256), ImVec2(0, 0), ImVec2(1, 1));
	ImGui::EndGroup();
	ImGui::End();


	// -----------------------------
	// [ Animation Info ]
	// -----------------------------
	ImGui::PushFont(m_DebugFont);
	ImGui::Begin("Animation Info");

	for (auto& mesh : m_SkeletalMeshes)
	{
		if (!mesh || !mesh->m_Asset) continue;

		// [ Mesh 이름 표시 ]
		ImGui::Text(" [ SkeletalMesh Name: %s ]", mesh->m_Name.c_str());

		// [ 현재 애니메이션 상태 표시 ]
		AnimationState* state = mesh->m_Controller.GetCurrentState();
		ImGui::Text(" Current Animation: ");          
		ImGui::SameLine();                           

		if (state)	ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s", state->Name.c_str());
		else		ImGui::TextColored(ImVec4(0.0f, 1.0f, 1.0f, 1.0f), "None");

		// [ Animation Playback Info ]
		Animator& animator = mesh->m_Controller.AnimatorInstance;
		const AnimationClip* clip = animator.GetCurrentClip();

		if (clip)
		{
			float currentTime = animator.GetCurrentTime();
			float duration = clip->Duration;

			ImGui::Text(" Animation Time:");
			ImGui::Text("   %.2f / %.2f sec", currentTime, duration);

			float normalized = (duration > 0.0f)
				? fmod(currentTime, duration) / duration
				: 0.0f;

			ImGui::ProgressBar(normalized, ImVec2(200, 0));

			// 블렌딩 중 표시
			if (animator.GetNextClip())
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), " Blending...");
			}
		}
		else
		{
			ImGui::Text(" Animation Time: None");
		}

		// [ Bool parameters ]
		for (auto& param : mesh->m_Controller.Params.GetAllBools())
			ImGui::Text("   %s: %s", param.first.c_str(), param.second ? "true" : "false");

		// [ Float parameters ]
		for (auto& param : mesh->m_Controller.Params.GetAllFloats())
			ImGui::Text("   %s: %.2f", param.first.c_str(), param.second);

		ImGui::Text("");
		

		// [ Animation Flow ]
		ImGui::Text(" Animation Flow:");

		if (mesh->m_Name == "Human_1")
		{
			ImGui::BulletText("Dance_1 : play for 5.0 sec -> transition to Dance_2");
			ImGui::BulletText("Dance_2 : play once        -> transition to Dance_1");
		}
		else if (mesh->m_Name == "Human_3")
		{
			ImGui::Text("Select Animation");

			bool idle = mesh->m_Controller.Params.GetBool("Idle");
			bool attack = mesh->m_Controller.Params.GetBool("Attack");
			bool die = mesh->m_Controller.Params.GetBool("Die");

			if (ImGui::RadioButton("Idle", idle))
			{
				mesh->m_Controller.Params.SetBool("Idle", true);
				mesh->m_Controller.Params.SetBool("Attack", false);
				mesh->m_Controller.Params.SetBool("Die", false);
			}

			if (ImGui::RadioButton("Attack", attack))
			{
				mesh->m_Controller.Params.SetBool("Idle", false);
				mesh->m_Controller.Params.SetBool("Attack", true);
				mesh->m_Controller.Params.SetBool("Die", false);
			}

			if (ImGui::RadioButton("Die", die))
			{
				mesh->m_Controller.Params.SetBool("Idle", false);
				mesh->m_Controller.Params.SetBool("Attack", false);
				mesh->m_Controller.Params.SetBool("Die", true);
			}
		}
		else
		{
			ImGui::Text(" (No animation flow description)");
		}

		ImGui::Text("");
		ImGui::Separator();
		ImGui::Text("");
	}
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
	if (!m_DrawShadowFrustum) return;

	auto* context = m_D3DDevice.GetDeviceContext();

	context->OMSetDepthStencilState(m_pDSState_DebugDraw.Get(), 0);

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

	DrawPhysXActors();

	m_DebugBatch->End();

	m_D3DDevice.GetDeviceContext()->OMSetDepthStencilState(nullptr, 0);
}

void TestApp::DrawPhysXActors()
{
	PxScene* scene = PhysicsSystem::Get().GetScene();
	if (!scene) return;

	PxU32 actorCount = scene->getNbActors(
		PxActorTypeFlag::eRIGID_STATIC |
		PxActorTypeFlag::eRIGID_DYNAMIC
	);

	std::vector<PxActor*> actors(actorCount);
	scene->getActors(
		PxActorTypeFlag::eRIGID_STATIC |
		PxActorTypeFlag::eRIGID_DYNAMIC,
		actors.data(),
		actorCount
	);

	// [ Actor ] 
	for (PxActor* actor : actors)
	{
		PxRigidActor* rigid = actor->is<PxRigidActor>();
		if (!rigid) return;

		PxTransform actorPose = rigid->getGlobalPose();

		PxU32 shapeCount = rigid->getNbShapes();
		std::vector<PxShape*> shapes(shapeCount);
		rigid->getShapes(shapes.data(), shapeCount);

		for (PxShape* shape : shapes)
		{
			// [ Shape ]
			DrawPhysXShape(shape, actorPose);
		}
	}

	// Character Controller는 별도로 그려야 함 
	DrawCharacterControllers();
}

void TestApp::DrawPhysXShape(PxShape* shape, const PxTransform& actorPose)
{
	PxGeometryHolder geo = shape->getGeometry();
	PxTransform localPose = shape->getLocalPose();
	PxTransform worldPose = actorPose * localPose;

	switch (geo.getType())
	{
	case PxGeometryType::eBOX:
	{
		const PxBoxGeometry& box = geo.box();

		DirectX::BoundingOrientedBox obb;
		obb.Center = {
			worldPose.p.x,
			worldPose.p.y,
			worldPose.p.z
		};
		obb.Extents = {
			box.halfExtents.x,
			box.halfExtents.y,
			box.halfExtents.z
		};
		obb.Orientation = {
			worldPose.q.x,
			worldPose.q.y,
			worldPose.q.z,
			worldPose.q.w
		};

		m_DebugDraw->Draw(m_DebugBatch.get(), obb, DirectX::Colors::Green);
		break;
	}

	case PxGeometryType::eSPHERE:
	{
		const PxSphereGeometry& sphere = geo.sphere();

		DirectX::BoundingSphere bs;
		bs.Center = {
			worldPose.p.x,
			worldPose.p.y,
			worldPose.p.z
		};
		bs.Radius = sphere.radius;

		m_DebugDraw->Draw(m_DebugBatch.get(), bs, DirectX::Colors::Yellow);
		break;
	}

	case PxGeometryType::eCAPSULE:
	{
		const PxCapsuleGeometry& capsule = geo.capsule();

		m_DebugDraw->DrawCapsule(
			m_DebugBatch.get(),
			worldPose.p,
			capsule.radius,
			capsule.halfHeight * 2.0f,
			DirectX::Colors::Cyan,
			worldPose.q
		);
		break;
	}

	default:
		break;
	}
}

void TestApp::DrawCharacterControllers()
{
	PxControllerManager* mgr = PhysicsSystem::Get().GetControllerManager();
	if (!mgr) return;

	PxU32 count = mgr->getNbControllers();

	for (PxU32 i = 0; i < count; ++i)
	{
		PxController* cct = mgr->getController(i);
		if (!cct) continue;

		// PhysX 5 방식 타입 체크
		if (cct->getType() != PxControllerShapeType::eCAPSULE)
			continue;

		PxCapsuleController* capsule =
			static_cast<PxCapsuleController*>(cct);

		PxExtendedVec3 p = capsule->getPosition();

		m_DebugDraw->DrawCapsule(
			m_DebugBatch.get(),
			PxVec3((float)p.x, (float)p.y, (float)p.z),
			capsule->getRadius(),
			capsule->getHeight(),
			DirectX::Colors::Red
		);
	}
}



void TestApp::UninitScene()
{
	OutputDebugString(L"[TestApp::UninitScene] 실행\r\n");

	// 샘플러, 상수버퍼, 셰이더, 입력레이아웃 해제
	m_pSamplerLinear.Reset();
	m_pSamplerComparison.Reset();
	m_pConstantBuffer.Reset();
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
	m_pSamplerIBL.Reset();
	m_pSamplerIBL_Clamp.Reset();

	m_ToneMapCB.Reset();
	m_pToneMapVS.Reset();
	m_pToneMapPS_HDR.Reset();
	m_pToneMapPS_LDR.Reset();
	m_HDRSceneTex.Reset();
	m_HDRSceneRTV.Reset();
	m_HDRSceneSRV.Reset();

	m_pDSState_DebugDraw.Reset();

	for (int i = 0; i < GBufferCount; ++i)
	{
		m_GBufferSRV[i].Reset();
		m_GBufferRTV[i].Reset();
		m_GBufferTex[i].Reset();
	}

	m_pDSState_GBuffer.Reset();
	m_pDSState_Deferred.Reset();
	m_pGBufferVS.Reset();
	m_pGBufferPS.Reset();
	m_pDeferredLightingPS.Reset();

	// 인스턴스 해제
	m_IBLSet.clear();
	m_StaticMeshes.clear();
	m_SkeletalMeshes.clear();

	humanAsset.reset();
	charAsset.reset();
	planeAsset.reset();
	treeAsset.reset();
	CharacterAsset.reset();

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
	m_DebugFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 13.5f);

	// 3. Title 폰트 
	TitleFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 15.0f); 

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