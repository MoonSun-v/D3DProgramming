#include "TestApp.h"

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

	m_Camera.m_Position = Vector3(0.0f, 80.0f, -500.0f);
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
	Matrix world =
		XMMatrixScaling(m_CharScale.x, m_CharScale.y, m_CharScale.z) *   // 스케일
		XMMatrixRotationRollPitchYaw(rotX, rotY, rotZ) *                 // 입력 회전
		XMMatrixTranslation(m_CharPos[0], m_CharPos[1], m_CharPos[2]);  // 위치
	m_WorldHuman = world; 

	m_WorldVampire = XMMatrixTranslation(m_VampirePos[0], m_VampirePos[1], m_VampirePos[2]);

	m_WorldPlane =
		XMMatrixScaling(m_PlaneScale.x, m_PlaneScale.y, m_PlaneScale.z) *
		XMMatrixTranslation(m_PlanePos[0], m_PlanePos[1], m_PlanePos[2]);

	
	// ---------------------------------------------
	// [ Shadow 카메라 위치 계산 (원근 투영) ]
	// ---------------------------------------------
	
	// [ ShadowLookAt ]
	float shadowDistance = 200.0f; // 카메라 앞쪽 거리
	Vector3 shadowLookAt = m_Camera.m_Position + m_Camera.GetForward() * shadowDistance;

	// [ ShadowPos ] 라이트 방향을 기준으로 카메라 위치 계산 
	float shadowBackOffset = 80.0f;  // 라이트가 어느 정도 떨어진 곳에서 바라보게
	Vector3 lightDir = XMVector3Normalize(Vector3(m_LightDir.x, m_LightDir.y, m_LightDir.z));
	Vector3 shadowPos = shadowLookAt - lightDir * shadowBackOffset;

	// [ Shadow View ]
	m_ShadowView = XMMatrixLookAtLH( shadowPos, shadowLookAt, Vector3(0.0f, 1.0f, 0.0f) );

	// Projection 행렬 (Perspective 원근 투영) : fov, aspect, nearZ, farZ
	m_ShadowProjection = XMMatrixPerspectiveFovLH( XM_PIDIV4, m_ShadowViewport.Width / (FLOAT)m_ShadowViewport.Height, 1.0f, 500.f );


	// [ 오브젝트 업데이트 ]

	Human.Update(deltaTime, world);
	Vampire.Update(deltaTime, m_WorldVampire);
	Plane.Update(deltaTime, m_WorldPlane);
}     


// ★ [ 렌더링 ] 
// PixelShader에서 ShadowMap SRV와 Comparison Sampler를 사용해서 그림자 샘플링
// 
void TestApp::Render()
{
	// 1. ShadowMap 렌더링 (DepthOnlyPass)
	// PS에서 ShadowMap SRV를 null로 언바인딩
	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	m_D3DDevice.GetDeviceContext()->PSSetShaderResources(5, 1, nullSRV);

	// ShadowMap 렌더링
	RenderShadowMap();

	// 화면 초기화
	const float clearColor[4] = { m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w };
	m_D3DDevice.BeginFrame(clearColor);

	m_D3DDevice.GetDeviceContext()->IASetInputLayout(m_pInputLayout.Get());
	m_D3DDevice.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_D3DDevice.GetDeviceContext()->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());
	m_D3DDevice.GetDeviceContext()->PSSetSamplers(1, 1, m_pSamplerComparison.GetAddressOf()); 

	// ShadowMap SRV 바인딩
	m_D3DDevice.GetDeviceContext()->PSSetShaderResources(5, 1, m_pShadowMapSRV.GetAddressOf());

	// [ Mesh 렌더링 ]
	RenderMesh(Human, m_WorldHuman, 0);
	RenderMesh(Vampire, m_WorldVampire, 0);
	RenderMesh(Plane, m_WorldPlane, 1);

	// UI 그리기 
	Render_ImGui();

	// 스왑체인 교체 (화면 출력 : 백 버퍼 <-> 프론트 버퍼 교체)
	m_D3DDevice.EndFrame(); 
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

	// GPU 업로드
	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_D3DDevice.GetDeviceContext()->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	if (isRigid == 1)
	{
		// StaticMesh일 경우: 이전 본 버퍼(b1, b2) 언바인딩
		ID3D11Buffer* nullCB[1] = { nullptr };
		m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(1, 1, nullCB);
		m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(2, 1, nullCB);
	}

	// SkeletalMesh에서 b1, b2 업데이트
	mesh.Render(m_D3DDevice.GetDeviceContext(), m_pSamplerLinear.Get());
}


// [ Multi Pass Rendering ] 
// 1.DepthOnly : Shadow Pass -> Depth 생성 (Depth만 렌더링, PS 필요X)
// 2.Render    : Main Pass -> SRV 샘플링 -> shadowFactor 계산 -> directLighting 곱

void TestApp::RenderShadowMap()
{
	// 1) ShadowMap DSV로 렌더 타겟 설정
	m_D3DDevice.GetDeviceContext()->OMSetRenderTargets(0, nullptr, m_pShadowMapDSV.Get());

	// 2) Shadow 뷰포트 설정
	m_D3DDevice.GetDeviceContext()->RSSetViewports(1, &m_ShadowViewport);

	// 3) Shadow 상수버퍼 
	ShadowConstantBuffer shadowCB;
	shadowCB.mLightView = XMMatrixTranspose(m_ShadowView);
	shadowCB.mLightProjection = XMMatrixTranspose(m_ShadowProjection);

	// 4) VertexShader만 설정 (Depth만 필요)
	m_D3DDevice.GetDeviceContext()->IASetInputLayout(m_pShadowInputLayout.Get());
	m_D3DDevice.GetDeviceContext()->VSSetShader(m_pShadowVS.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetShader(nullptr, nullptr, 0);

	
	// ------------------------------
	// 각 메시 Depth 렌더링 : ShadowMap에 기록 
	// ------------------------------
	// Human
	shadowCB.mWorld = XMMatrixTranspose(m_WorldHuman);
	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pShadowCB.Get(), 0, nullptr, &shadowCB, 0, 0);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(5, 1, m_pShadowCB.GetAddressOf());
	Human.Render(m_D3DDevice.GetDeviceContext(), nullptr);

	// Vampire
	shadowCB.mWorld = XMMatrixTranspose(m_WorldVampire);
	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pShadowCB.Get(), 0, nullptr, &shadowCB, 0, 0);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(5, 1, m_pShadowCB.GetAddressOf());
	Vampire.Render(m_D3DDevice.GetDeviceContext(), nullptr);

	// Plane
	shadowCB.mWorld = XMMatrixTranspose(m_WorldPlane);
	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pShadowCB.Get(), 0, nullptr, &shadowCB, 0, 0);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(5, 1, m_pShadowCB.GetAddressOf());
	Plane.Render(m_D3DDevice.GetDeviceContext(), nullptr);

	// 5) 메인 Pass 렌더타겟/뷰포트 복원
	m_D3DDevice.GetDeviceContext()->RSSetViewports(1, &m_D3DDevice.GetViewport());
	ID3D11RenderTargetView* rtv = m_D3DDevice.GetRenderTargetView();
	m_D3DDevice.GetDeviceContext()->OMSetRenderTargets(1, &rtv, m_D3DDevice.GetDepthStencilView());
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


	// -----------------------------
	// [ Object Transform ]
	// -----------------------------
	ImGui::Text(" ");
	ImGui::Text("[ Object Transform ]");

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
	// [ Light ]
	// -----------------------------
	ImGui::Text("[ Light ]");

	// 광원 색상
	ImGui::ColorEdit3("Light Color", (float*)&m_LightColor);

	// 광원 방향 
	ImGui::DragFloat3("Light Dir", (float*)&m_LightDir, 0.01f, -1.0f, 1.0f);

	// 주변광 / 난반사 / 정반사 계수
	ImGui::ColorEdit3("Ambient Light", (float*)&m_LightAmbient);
	ImGui::ColorEdit3("Diffuse Light", (float*)&m_LightDiffuse);
	ImGui::DragFloat("Shininess (alpha)", &m_Shininess, 10.0f, 5.0f, 1000.0f);

	ImGui::Separator();
	ImGui::Text("");


	// -----------------------------
	// [ Material ]
	// -----------------------------
	ImGui::Text("[ Material ]");

	ImGui::ColorEdit3("Ambient", (float*)&m_MaterialAmbient);
	ImGui::ColorEdit3("Specular", (float*)&m_MaterialSpecular);

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
	ImGui::End();

	// [ ImGui 프레임 최종 렌더링 ]
	ImGui::Render();  // ImGui 내부에서 렌더링 데이터를 준비
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // DX11로 실제 화면에 출력
}


// ★ [ Scene 초기화 ] 
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
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	HR_T(m_D3DDevice.GetDevice()->CreateInputLayout(shadowLayout, ARRAYSIZE(shadowLayout), ShadowVSBuffer->GetBufferPointer(), ShadowVSBuffer->GetBufferSize(), m_pShadowInputLayout.GetAddressOf()));


	// ---------------------------------------------------------------
	// 픽셀 셰이더(Pixel Shader) 컴파일 및 생성
	// ---------------------------------------------------------------
	ComPtr<ID3DBlob> pixelShaderBuffer; 
	HR_T(CompileShaderFromFile(L"../Shaders/12.PixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));


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
	Human.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/SkinningTest.fbx");
	Vampire.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/Vampire_SkinningTest.fbx");
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
	//sampDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
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


	// ---------------------------------------------------------------
	// 샘플러 생성
	// ---------------------------------------------------------------
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
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4,m_ClientWidth / (FLOAT)m_ClientHeight, m_CameraNear, m_CameraFar);


	return true;
}

void TestApp::UninitScene()
{
	// 샘플러, 상수버퍼, 셰이더, 입력레이아웃 해제
	m_pSamplerLinear.Reset();
	m_pConstantBuffer.Reset();
	m_pVertexShader.Reset();
	m_pPixelShader.Reset();
	m_pInputLayout.Reset();

	// Mesh, Material 해제
	Human.Clear();
	Vampire.Clear();
	Plane.Clear();

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