#include "TestApp.h"
#include "AssetManager.h"

#include <string> 
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <Directxtk/DDSTextureLoader.h>
#include <windows.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "Psapi.lib")


TestApp::TestApp() : GameApp()
{
}

TestApp::~TestApp()
{
}

bool TestApp::Initialize()
{
	__super::Initialize();

	if (!m_D3DDevice.Initialize(m_hWnd, m_ClientWidth, m_ClientHeight)) return false;
	if (!InitScene())	return false;
	if (!InitImGUI())	return false;

	m_Camera.m_Position = Vector3(40.0f, 80.0f, -600.0f);
	m_Camera.m_Rotation = Vector3(0.0f, 0.0f, 0.0f);
	m_Camera.SetSpeed(200.0f);

	return true;
}

void TestApp::Uninitialize()
{
	UninitImGUI();
	m_D3DDevice.Cleanup();
	UninitScene();      // 리소스 해제 
	CheckDXGIDebug();	// DirectX 리소스 누수 체크
}

void TestApp::Update()
{
	__super::Update();

	float deltaTime = TimeSystem::m_Instance->DeltaTime();
	float totalTime = TimeSystem::m_Instance->TotalTime();

	m_Camera.GetViewMatrix(m_View);			// View 행렬 갱신

	// [ 오브젝트 ] 
	m_WorldHuman =
		XMMatrixScaling(m_CharScale.x, m_CharScale.y, m_CharScale.z) *   // 스케일
		XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ) *                 // 입력 회전
		XMMatrixTranslation(m_CharPos[0], m_CharPos[1], m_CharPos[2]);   // 위치

	m_WorldVampire = XMMatrixTranslation(m_VampirePos[0], m_VampirePos[1], m_VampirePos[2]);

	m_WorldPlane =
		XMMatrixScaling(m_PlaneScale.x, m_PlaneScale.y, m_PlaneScale.z) *
		XMMatrixTranslation(m_PlanePos[0], m_PlanePos[1], m_PlanePos[2]);

	m_WorldCube =
		XMMatrixScaling(m_CubeScale.x, m_CubeScale.y, m_CubeScale.z) *
		XMMatrixTranslation(m_CubePos[0], m_CubePos[1], m_CubePos[2]);

	m_WorldTree =
		XMMatrixScaling(m_TreeScale.x, m_TreeScale.y, m_TreeScale.z) *
		XMMatrixTranslation(m_TreePos[0], m_TreePos[1], m_TreePos[2]);
	

	// [ 오브젝트 업데이트 ]
	Plane.Update(deltaTime, m_WorldPlane);
	for (size_t i = 0; i < m_Humans.size(); i++)
	{
		m_Humans[i]->Update(deltaTime, m_HumansWorld[i]);
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

	// [ Shadow View ]
	m_ShadowView = XMMatrixLookAtLH(shadowPos, shadowLookAt, Vector3(0.0f, 1.0f, 0.0f));

	// Projection 행렬 (Perspective 원근 투영) : fov, aspect, nearZ, farZ
	m_ShadowProjection = XMMatrixPerspectiveFovLH(1.5f/*XM_PIDIV4*/, m_ShadowViewport.Width / (FLOAT)m_ShadowViewport.Height, 500.0f, 10000.f);

}     

void TestApp::Render()
{
	// [ DepthOnly Pass 렌더링 (ShadowMap) ]
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	m_D3DDevice.GetDeviceContext()->PSSetShaderResources(5, 1, nullSRV);

	RenderShadowMap();

	// [ Main Pass 렌더링 ]
	const float clearColor[4] = { m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w };
	m_D3DDevice.BeginFrame(clearColor);

	auto* context = m_D3DDevice.GetDeviceContext();

	context->IASetInputLayout(m_pInputLayout.Get());
	context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	context->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	context->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	context->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
	context->PSSetSamplers(1, 1, m_pSamplerComparison.GetAddressOf());

	// ShadowMap SRV 바인딩
	context->PSSetShaderResources(5, 1, m_pShadowMapSRV.GetAddressOf());

	// -----------------------
	// Mesh 렌더링
	// -----------------------
	RenderMesh(Plane, m_WorldPlane, 1);          // Static Mesh
	// RenderMesh(Human, m_WorldHuman, 0);         // SkeletalMeshAsset 기반 단일 Human

	// SkeletalMeshInstance 배열
	for (size_t i = 0; i < m_Humans.size(); i++)
	{
		RenderMeshInstance(*m_Humans[i], m_HumansWorld[i]);
	}

	// UI 렌더링
	Render_ImGui();

	// 화면 출력
	m_D3DDevice.EndFrame();
}

// -----------------------
// SkeletalMeshInstance용 Render
// -----------------------
void TestApp::RenderMeshInstance(SkeletalMeshInstance& instance, const Matrix& world)
{
	auto* context = m_D3DDevice.GetDeviceContext();

	// b0 업데이트
	cb.mWorld = XMMatrixTranspose(world);
	cb.mView = XMMatrixTranspose(m_View);
	cb.mProjection = XMMatrixTranspose(m_Projection);
	cb.vLightDir = m_LightDir;
	cb.vLightColor = m_LightColor;
	cb.vEyePos = XMFLOAT4(m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z, 1.0f);
	cb.vAmbient = m_MaterialAmbient;
	cb.vDiffuse = m_LightDiffuse;
	cb.vSpecular = m_MaterialSpecular;
	cb.fShininess = m_Shininess;
	cb.gIsRigid = 0; // SkeletalMeshInstance는 스켈레탈

	context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	// Render 호출
	instance.Render(context, m_pSamplerLinear.Get(), 0);
}

// -----------------------
// Shadow Map 렌더링
// -----------------------
void TestApp::RenderShadowMap()
{
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

	auto RenderShadowObject = [&](SkeletalMesh& mesh, const Matrix& world, int isRigid)
		{
			cb.mWorld = XMMatrixTranspose(world);
			cb.gIsRigid = isRigid;
			context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

			shadowCB.mWorld = XMMatrixTranspose(world);
			context->UpdateSubresource(m_pShadowCB.Get(), 0, nullptr, &shadowCB, 0, 0);

			mesh.RenderShadow(context, isRigid);
		};

	auto RenderShadowInstance = [&](SkeletalMeshInstance& instance, const Matrix& world)
		{
			cb.mWorld = XMMatrixTranspose(world);
			cb.gIsRigid = 0;
			context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

			shadowCB.mWorld = XMMatrixTranspose(world);
			context->UpdateSubresource(m_pShadowCB.Get(), 0, nullptr, &shadowCB, 0, 0);

			instance.RenderShadow(context, 0);
		};

	// Static / Skeletal 렌더
	RenderShadowObject(Plane, m_WorldPlane, 1);
	// RenderShadowObject(Human, m_WorldHuman, 0);

	for (size_t i = 0; i < m_Humans.size(); i++)
		RenderShadowInstance(*m_Humans[i], m_HumansWorld[i]);

	// RenderShadowObject(Vampire, m_WorldVampire, 0);
	// RenderShadowObject(cube, m_WorldCube, 1);
	// RenderShadowObject(Tree, m_WorldTree, 1);

	// RenderTarget / Viewport 복원
	context->RSSetViewports(1, &m_D3DDevice.GetViewport());
	ID3D11RenderTargetView* rtv = m_D3DDevice.GetRenderTargetView();
	context->OMSetRenderTargets(1, &rtv, m_D3DDevice.GetDepthStencilView());
}

void TestApp::RenderMesh(SkeletalMesh& mesh, const Matrix& world, int isRigid)
{
	cb.mWorld = XMMatrixTranspose(world);
	cb.mView = XMMatrixTranspose(m_View);
	cb.mProjection = XMMatrixTranspose(m_Projection);
	cb.vLightDir = m_LightDir;
	cb.vLightColor = m_LightColor;
	cb.vEyePos = XMFLOAT4(m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z, 1.0f);
	cb.vAmbient = m_MaterialAmbient;
	cb.vDiffuse = m_LightDiffuse;
	cb.vSpecular = m_MaterialSpecular;
	cb.fShininess = m_Shininess;
	cb.gIsRigid = isRigid;

	// b0
	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_D3DDevice.GetDeviceContext()->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	// StaticMesh일 경우: 이전 본 버퍼(b1, b2) 언바인딩
	if (isRigid == 1)
	{
		ID3D11Buffer* nullCB[1] = { nullptr };
		m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(1, 1, nullCB);
		m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(2, 1, nullCB);
	}

	// SkeletalMesh에서 b1, b2 업데이트 
	mesh.Render(m_D3DDevice.GetDeviceContext(), m_pSamplerLinear.Get(), isRigid);
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
	HR_T(CompileShaderFromFile(L"../Shaders/12.VertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));

	ComPtr<ID3DBlob> ShadowVSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/12.ShadowVertexShader.hlsl", "ShadowVS", "vs_4_0", ShadowVSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(ShadowVSBuffer->GetBufferPointer(), ShadowVSBuffer->GetBufferSize(), NULL, m_pShadowVS.GetAddressOf()));

	// ---------------------------------------------------------------
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
	HR_T(CompileShaderFromFile(L"../Shaders/12.PixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));

	ComPtr<ID3DBlob> ShadowPSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/12.ShadowPixelShader.hlsl", "ShadowPS", "ps_4_0", ShadowPSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(ShadowPSBuffer->GetBufferPointer(), ShadowPSBuffer->GetBufferSize(), NULL, m_pShadowPS.GetAddressOf()));


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
	// 리소스 로드 
	// ---------------------------------------------------------------
	// Human.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/SkinningTest.fbx");
	
	// [ FBX 파일에서 SkeletalMeshAsset 생성 ]
	humanAsset = AssetManager::Get().LoadSkeletalMesh(m_D3DDevice.GetDevice(), "../Resource/SkinningTest.fbx");

	// SkeletalMeshInstance 생성 후 Asset 연결
	auto instance = std::make_shared<SkeletalMeshInstance>();
	instance->SetAsset(m_D3DDevice.GetDevice(), humanAsset);

	m_Humans.push_back(instance);
	m_HumansWorld.push_back(m_WorldHuman);


	Plane.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/Plane.fbx");


	// ---------------------------------------------------------------
	// Shadow Map 생성 
	// ---------------------------------------------------------------
	
	// Shadow Map 뷰포트 설정
	m_ShadowViewport.TopLeftX = 0.0f;
	m_ShadowViewport.TopLeftY = 0.0f;
	m_ShadowViewport.Width = 8192.0f; // 해상도
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
	
	// D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HR_T(m_D3DDevice.GetDevice()->CreateSamplerState(&sampDesc, m_pSamplerLinear.GetAddressOf()));


	// ---------------------------------------------------------------
	// 행렬(World, View, Projection) 설정
	// ---------------------------------------------------------------
	m_World = XMMatrixIdentity(); // 단위 행렬 

	// 카메라(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// 카메라 위치
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라가 바라보는 위치
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라의 위쪽 방향 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// 왼손 좌표계(LH) 기준 

	// 투영행렬(Projection) : 카메라의 시야각(FOV), 화면 종횡비, Near, Far 
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, m_ClientWidth / (FLOAT)m_ClientHeight, m_CameraNear, m_CameraFar);


	return true;
}

void TestApp::AddHumanInFrontOfCamera()
{
	// Asset으로부터 새 인스턴스 생성
	auto newHuman = std::make_shared<SkeletalMeshInstance>();
	newHuman->SetAsset(m_D3DDevice.GetDevice(), humanAsset);

    // 카메라 정보
	Vector3 camPos = m_Camera.m_Position;
	Vector3 camForward = m_Camera.GetForward();

	// 생성 위치 계산
	float spawnDist = 2.0f;   // 카메라 5m 앞
	float offsetRange = 1.0f; // 양옆 랜덤 편차

	float offsetX = ((rand() % 1000) / 1000.0f - 0.5f) * offsetRange * 2.0f;
	float offsetZ = ((rand() % 1000) / 1000.0f - 0.5f) * offsetRange * 2.0f;

	Vector3 spawnPos = camPos + camForward * spawnDist + Vector3(offsetX, 0.0f, offsetZ);

	Matrix world = XMMatrixScaling(1, 1, 1) * XMMatrixTranslation(spawnPos.x, spawnPos.y, spawnPos.z);


	// 벡터에 추가
	m_Humans.push_back(newHuman);
	m_HumansWorld.push_back(world);

	OutputDebugString((L"AddHumanInFrontOfCamera called! Total humans: " + std::to_wstring(m_Humans.size()) + L"\n").c_str());
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
	ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_Always); // ImGuiCond_FirstUseEver

	// [ Control UI ]
	ImGui::Begin("Controllor");

	// UI용 폰트 적용
	ImGui::PushFont(m_UIFont);

	// -----------------------------
	// [ Object Transform ]
	// -----------------------------
	ImGui::Text(" ");
	ImGui::Text("[ Human Object Transform ]");

	// 위치
	ImGui::Text("Position (XYZ)");
	ImGui::DragFloat3("Object_Position", m_CharPos, 1.0f);

	// 스케일
	ImGui::Text("Scale (XYZ)");
	ImGui::DragFloat3("Object_Scale", (float*)&m_CharScale, 0.01f, 0.01f, 10.0f);

	// 회전 (도 단위)
	static float rotDegree[3] = { XMConvertToDegrees(rotX), XMConvertToDegrees(rotY), XMConvertToDegrees(rotZ) };
	ImGui::Text("Rotation (Degrees)");
	if (ImGui::DragFloat3("Object_Rotation", rotDegree, 0.5f, -360.0f, 360.0f))
	{
		// 입력값을 라디안으로 변환 적용
		rotX = XMConvertToRadians(rotDegree[0]);
		rotY = XMConvertToRadians(rotDegree[1]);
		rotZ = XMConvertToRadians(rotDegree[2]);
	}

	ImGui::Separator();
	ImGui::Text("");

	// -----------------------------
	// [ Camera ]
	// -----------------------------
	ImGui::Text("[ Camera ]");

	// 카메라 위치
	ImGui::Text("Position (XYZ)");
	if (ImGui::DragFloat3("Position", &m_Camera.m_Position.x, 0.5f))
	{
		// 위치 변경 시 즉시 World 행렬 업데이트
		m_Camera.m_World = Matrix::CreateFromYawPitchRoll(m_Camera.m_Rotation) * Matrix::CreateTranslation(m_Camera.m_Position);
	}

	// 카메라 회전 (도 단위)
	Vector3 rotationDegree =
	{
		XMConvertToDegrees(m_Camera.m_Rotation.x),
		XMConvertToDegrees(m_Camera.m_Rotation.y),
		XMConvertToDegrees(m_Camera.m_Rotation.z)
	};

	ImGui::Text("Rotation (Degrees)");
	if (ImGui::DragFloat3("Rotation", &rotationDegree.x, 0.5f))
	{
		// 입력값을 라디안으로 변환하여 카메라에 적용
		m_Camera.m_Rotation.x = XMConvertToRadians(rotationDegree.x);
		m_Camera.m_Rotation.y = XMConvertToRadians(rotationDegree.y);
		m_Camera.m_Rotation.z = XMConvertToRadians(rotationDegree.z);

		// 회전 변경 시 World 행렬 갱신
		m_Camera.m_World = Matrix::CreateFromYawPitchRoll(m_Camera.m_Rotation) * Matrix::CreateTranslation(m_Camera.m_Position);
	}

	// 이동 속도 조절
	ImGui::Text("Move Speed");
	ImGui::SliderFloat("Speed", &m_Camera.m_MoveSpeed, 10.0f, 1000.0f, "%.1f");

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

	if (ImGui::Button("Add Human"))
	{
		AddHumanInFrontOfCamera();
	}

	// FPS 출력
	// ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

	// [ 시스템 메모리 사용량 ]
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);

	double totalPhysGB = memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
	double usedPhysGB = (memInfo.ullTotalPhys - memInfo.ullAvailPhys) / (1024.0 * 1024.0 * 1024.0);

	ImGui::Text("System RAM: %.1f / %.1f GB", usedPhysGB, totalPhysGB);


	// [ 페이지 파일 사용량 ]
	double totalPageGB = memInfo.ullTotalPageFile / (1024.0 * 1024.0 * 1024.0);
	double usedPageGB = (memInfo.ullTotalPageFile - memInfo.ullAvailPageFile) / (1024.0 * 1024.0 * 1024.0);

	ImGui::Text("Page File: %.1f / %.1f GB", usedPageGB, totalPageGB);


	// [ GPU VRAM ]
	IDXGIDevice* dxgiDevice = nullptr;
	IDXGIAdapter* adapter = nullptr;
	DXGI_ADAPTER_DESC desc;

	m_D3DDevice.GetDevice()->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
	dxgiDevice->GetAdapter(&adapter);

	adapter->GetDesc(&desc);
	UINT64 totalVRAM = desc.DedicatedVideoMemory;

	ImGui::Text("GPU VRAM: %.1f GB", totalVRAM / (1024.0 * 1024.0 * 1024.0));


	// 정리
	if (adapter)     adapter->Release();
	if (dxgiDevice)  dxgiDevice->Release();



	// 글자색과 폰트 복원
	ImGui::PopFont();
	ImGui::PopStyleColor();

	ImGui::End();


	// [ ImGui 프레임 최종 렌더링 ]
	ImGui::Render();  // ImGui 내부에서 렌더링 데이터를 준비
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // DX11로 실제 화면에 출력
}


void TestApp::UninitScene()
{
	// 샘플러, 상수버퍼, 셰이더, 입력레이아웃 해제
	m_pSamplerLinear.Reset();
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

	// Mesh, Material 해제
	// Human.Clear();
	// Vampire.Clear();
	Plane.Clear();
	// cube.Clear();
	// Tree.Clear();

	// 기본 텍스처 해제
	Material::DestroyDefaultTextures();
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