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

	return true;
}

void TestApp::Uninitialize()
{
	UninitScene();      // 리소스 해제 
	UninitImGUI();
	m_D3DDevice.Cleanup();
	CheckDXGIDebug();	// DirectX 리소스 누수 체크
}

void TestApp::Update()
{
	__super::Update();

	float deltaTime = TimeSystem::m_Instance->DeltaTime();
	float totalTime = TimeSystem::m_Instance->TotalTime();

	m_Camera.GetViewMatrix(m_View);			// View 행렬 갱신

	// [ 오브젝트 업데이트 ] : 각 오브젝트마다 별도 pos/scale 
	m_WorldPlane =
		XMMatrixScaling(m_PlaneScale.x, m_PlaneScale.y, m_PlaneScale.z) *
		XMMatrixTranslation(m_PlanePos[0], m_PlanePos[1], m_PlanePos[2]);

	m_WorldHuman =
		XMMatrixScaling(m_HumanScale.x, m_HumanScale.y, m_HumanScale.z) *
		XMMatrixTranslation(m_HumanPos[0], m_HumanPos[1], m_HumanPos[2]);

	if (m_Planes.size() > 0)
	{
		if (m_PlanesWorld.size() != m_Planes.size()) m_PlanesWorld.resize(m_Planes.size(), m_WorldPlane);

		for (size_t i = 0; i < m_Planes.size(); ++i)
		{
			m_PlanesWorld[i] = m_WorldPlane; // 같은 transform 적용. 필요하면 인덱스별 pos/scale 배열 사용 해야함 
		}
	}

	if (m_Humans.size() > 0)
	{
		if (m_HumansWorld.size() != m_Humans.size())
			m_HumansWorld.resize(m_Humans.size(), m_WorldHuman);

		for (size_t i = 0; i < m_Humans.size(); ++i)
		{
			m_HumansWorld[i] = m_WorldHuman; // 같은 transform 적용. 필요하면 인덱스별 pos/scale 배열 사용 해야함 
		} 
	}

	// 이제 인스턴스들 Update 호출 (업데이트된 world 전달)
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

void TestApp::UpdateConstantBuffer(const Matrix& world, int isRigid)
{
	cb.mWorld = XMMatrixTranspose(world);
	cb.mView = XMMatrixTranspose(m_View);
	cb.mProjection = XMMatrixTranspose(m_Projection);
	cb.vLightDir = m_LightDir;
	cb.vLightColor = m_LightColor;
	cb.vEyePos = XMFLOAT4(m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z, 1.0f);
	cb.gMetallicMultiplier = XMFLOAT4(1.0f, 0, 0, 0);  // 임시 값
	cb.gRoughnessMultiplier = XMFLOAT4(1.0f, 0, 0, 0); // 임시 값

	cb.gIsRigid = isRigid;

	auto* context = m_D3DDevice.GetDeviceContext();
	context->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	context->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	context->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
}


void TestApp::Render()
{
	// [ DepthOnly Pass 렌더링 (ShadowMap) ]
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	m_D3DDevice.GetDeviceContext()->PSSetShaderResources(6, 1, nullSRV);

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
	context->PSSetShaderResources(6, 1, m_pShadowMapSRV.GetAddressOf());


	// [ Mesh 렌더링 ]

	// Static Mesh Instance 
	for (size_t i = 0; i < m_Planes.size(); i++) { RenderStaticMesh(*m_Planes[i], m_PlanesWorld[i]); }

	// Skeletal Mesh Instance 
	for (size_t i = 0; i < m_Humans.size(); i++) { RenderSkeletalMesh(*m_Humans[i], m_HumansWorld[i]); }


	// UI 렌더링
	Render_ImGui();

	// 화면 출력
	m_D3DDevice.EndFrame();
}

void TestApp::RenderSkeletalMesh(SkeletalMeshInstance& instance, const Matrix& world)
{
	UpdateConstantBuffer(world, 0); // SkeletalMesh

	instance.Render(m_D3DDevice.GetDeviceContext(), m_pSamplerLinear.Get(), 0); // Render 호출
}

void TestApp::RenderStaticMesh(StaticMeshInstance& instance, const Matrix& world)
{
	UpdateConstantBuffer(world, 1); // StaticMesh

	instance.Render(m_D3DDevice.GetDeviceContext(), m_pSamplerLinear.Get());
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
	for (size_t i = 0; i < m_Planes.size(); i++)
	{
		RenderShadowStatic(*m_Planes[i], m_PlanesWorld[i]);
	}

	for (size_t i = 0; i < m_Humans.size(); i++)
	{
		RenderShadowSkeletal(*m_Humans[i], m_HumansWorld[i]);
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
	HR_T(CompileShaderFromFile(L"../Shaders/14.VertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));

	ComPtr<ID3DBlob> ShadowVSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/14.ShadowVertexShader.hlsl", "ShadowVS", "vs_4_0", ShadowVSBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(ShadowVSBuffer->GetBufferPointer(), ShadowVSBuffer->GetBufferSize(), NULL, m_pShadowVS.GetAddressOf()));

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
	HR_T(CompileShaderFromFile(L"../Shaders/14.PixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));

	ComPtr<ID3DBlob> ShadowPSBuffer;
	HR_T(CompileShaderFromFile(L"../Shaders/14.ShadowPixelShader.hlsl", "ShadowPS", "ps_4_0", ShadowPSBuffer.GetAddressOf()));
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
	// 리소스 로드 (Asset)
	// ---------------------------------------------------------------
	
	// [ Static Mesh Asset 생성 ]
	planeAsset = AssetManager::Get().LoadStaticMesh(m_D3DDevice.GetDevice(), "../Resource/Plane.fbx");

	auto instance_plane = std::make_shared<StaticMeshInstance>(); // StaticMeshInstance 생성 후 Asset 연결
	instance_plane->SetAsset(planeAsset);

	m_Planes.push_back(instance_plane);
	m_PlanesWorld.push_back(m_WorldPlane);


	// [ Skeletal Mesh Asset 생성 ]
	humanAsset = AssetManager::Get().LoadSkeletalMesh(m_D3DDevice.GetDevice(), "../Resource/SkinningTest.fbx");

	auto instance_human = std::make_shared<SkeletalMeshInstance>();
	instance_human->SetAsset(m_D3DDevice.GetDevice(), humanAsset);

	m_Humans.push_back(instance_human);
	m_HumansWorld.push_back(m_WorldHuman);


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

	m_Camera.m_Position = Vector3(0.0f, 100.0f, -500.0f);
	m_Camera.m_Rotation = Vector3(0.0f, 0.0f, 0.0f);
	m_Camera.SetSpeed(200.0f);

	// 카메라(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// 카메라 위치
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라가 바라보는 위치
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라의 위쪽 방향 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// 왼손 좌표계(LH) 기준 

	// 투영행렬(Projection) : 카메라의 시야각(FOV), 화면 종횡비, Near, Far 
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4, m_ClientWidth / (FLOAT)m_ClientHeight, m_CameraNear, m_CameraFar);


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
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_Always); // ImGuiCond_FirstUseEver

	// [ Control UI ]
	ImGui::Begin("Controllor");

	// UI용 폰트 적용
	ImGui::PushFont(m_UIFont);

	// -----------------------------
	// [ Light ]
	// -----------------------------
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

	// 카메라 위치
	ImGui::Text("Position (XYZ)");
	if (ImGui::DragFloat3("Position", &m_Camera.m_Position.x, 0.5f))
	{
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


	if (ImGui::Button("Reset Human & Trim"))
	{
		// Human 인스턴스 벡터 비우기
		m_Humans.clear();
		m_HumansWorld.clear();

		// Asset 강제 언로드
		if (humanAsset)
		{
			humanAsset->Clear();
			AssetManager::Get().UnloadSkeletalMesh("../Resource/SkinningTest.fbx");
			humanAsset.reset();
		}

		// IDXGIDevice3::Trim() 호출 (GPU 캐시 제거)
		IDXGIDevice3* dxgiDevice3 = nullptr;
		if (SUCCEEDED(m_D3DDevice.GetDevice()->QueryInterface(__uuidof(IDXGIDevice3), (void**)&dxgiDevice3)))
		{
			dxgiDevice3->Trim();
			dxgiDevice3->Release();
		}
	}

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


void TestApp::UninitScene()
{
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

	m_Humans.clear();
	m_Planes.clear();
	m_PlanesWorld.clear();

	OutputDebugString((L"[TestApp::UninitScene] 실행 "));

	AssetManager::Get().UnloadAll(); 

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