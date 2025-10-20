#include "TestApp.h"
#include "ConstantBuffer.h"

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
	UninitImGUI();
	m_D3DDevice.Cleanup();
	UninitScene();      // 리소스 해제 
	CheckDXGIDebug();	// DirectX 리소스 누수 체크
}

void TestApp::Update()
{
	__super::Update();

	float totalTime = TimeSystem::m_Instance->TotalTime();

	// [ 광원 처리 ]
	m_LightDirEvaluated = m_InitialLightDir; 

	// [ 카메라 갱신 ] 
	m_Camera.m_Position = { m_CameraPos[0], m_CameraPos[1], m_CameraPos[2] }; 
	m_Camera.m_Rotation.y = m_CameraYaw; 
	m_Camera.m_Rotation.x = m_CameraPitch; 

	m_Camera.Update(m_Timer.DeltaTime());	// 카메라 상태 업데이트 
	m_Camera.GetViewMatrix(m_View);			// View 행렬 갱신


	// [ 투영 행렬 갱신 ] : 카메라 Projection 행렬 갱신 (FOV, Near, Far 반영)
	m_Projection = XMMatrixPerspectiveFovLH
	(
		XMConvertToRadians(m_CameraFOV), // degree -> radian 변환
		float(m_ClientWidth) / float(m_ClientHeight),
		m_CameraNear,
		m_CameraFar
	);
}     


// ★ [ 렌더링 ] 
void TestApp::Render()
{
	// 화면 초기화
	const float clearColor[4] = { m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w };
	m_D3DDevice.BeginFrame(clearColor);

	m_D3DDevice.GetDeviceContext()->IASetInputLayout(m_pInputLayout.Get());
	m_D3DDevice.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_D3DDevice.GetDeviceContext()->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	// Mesh 렌더링
	auto RenderMesh = [&](StaticMesh& mesh, const Matrix& world)
	{
		ConstantBuffer cb;
		cb.mWorld = XMMatrixTranspose(world); // 각 오브젝트 위치 
		cb.mView = XMMatrixTranspose(m_View);
		cb.mProjection = XMMatrixTranspose(m_Projection);
		cb.vLightDir = m_LightDirEvaluated;
		cb.vLightColor = m_LightColor;
		cb.vEyePos = XMFLOAT4(m_CameraPos[0], m_CameraPos[1], m_CameraPos[2], 1.0f);
		cb.vAmbient = m_MaterialAmbient;
		cb.vDiffuse = m_LightDiffuse;
		cb.vSpecular = m_MaterialSpecular;
		cb.fShininess = m_Shininess;

		for (StaticMeshSection& sub : mesh.m_SubMeshes)
		{
			// Material 텍스처 바인딩
			const TextureSRVs& tex = mesh.m_Materials[sub.m_MaterialIndex].GetTextures();
			ID3D11ShaderResourceView* srvs[5] =
			{
				tex.DiffuseSRV.Get(),
				tex.NormalSRV.Get(),
				tex.SpecularSRV.Get(),
				tex.EmissiveSRV.Get(),
				tex.OpacitySRV.Get()
			};
			m_D3DDevice.GetDeviceContext()->PSSetShaderResources(0, 5, srvs);

			// SubMesh 렌더링
			sub.Render(m_D3DDevice.GetDeviceContext(), mesh.m_Materials[sub.m_MaterialIndex], cb, m_pConstantBuffer.Get(), m_pSamplerLinear.Get());
		}
	};
	
	RenderMesh(treeMesh, m_WorldTree);
	RenderMesh(charMesh, m_WorldChar);
	RenderMesh(zeldaMesh, m_WorldZelda);


	// UI 그리기 
	Render_ImGui();


	// 스왑체인 교체 (화면 출력 : 백 버퍼 <-> 프론트 버퍼 교체)
	m_D3DDevice.EndFrame(); 
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
	// ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver); 
	ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_Always);

	// [ Control UI ]
	ImGui::Begin("Controllor");


	// -----------------------------
	// [ Light ]
	// -----------------------------
	ImGui::Text(" ");
	ImGui::Text("[ Light ]");

	// 광원 색상
	ImGui::ColorEdit3("Light Color", (float*)&m_LightColor);

	// 광원 방향 
	ImGui::DragFloat3("Light Dir", (float*)&m_InitialLightDir, 0.01f, -1.0f, 1.0f);

	// 방향 벡터 정규화
	{
		XMVECTOR dir = XMLoadFloat4(&m_InitialLightDir);
		dir = XMVector3Normalize(dir);
		XMStoreFloat4(&m_LightDirEvaluated, dir);
	}

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
	ImGui::DragFloat3("Camera Pos", m_CameraPos, 1.0f, -1000.0f, 1000.0f);

	// 카메라 회전 (Yaw / Pitch)
	ImGui::DragFloat("Yaw", &m_CameraYaw, 0.01f);
	ImGui::DragFloat("Pitch", &m_CameraPitch, 0.01f);

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
	HR_T(CompileShaderFromFile(L"../Shaders/09.VertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));


	// ---------------------------------------------------------------
	// 입력 레이아웃(Input Layout) 생성
	//---------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // POSITION : float3 (12바이트)
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },  // NORMAL   : float3 (12바이트, 오프셋 12)
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // TEXCOORD : float2 (8바이트, 오프셋 24)
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	HR_T(m_D3DDevice.GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));


	// ---------------------------------------------------------------
	// 픽셀 셰이더(Pixel Shader) 컴파일 및 생성
	// ---------------------------------------------------------------
	ComPtr<ID3DBlob> pixelShaderBuffer; 
	HR_T(CompileShaderFromFile(L"../Shaders/09.PixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));


	// ---------------------------------------------------------------
	// 상수 버퍼(Constant Buffer) 생성
	// ---------------------------------------------------------------
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer); // World, View, Projection + Light
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&bd, nullptr, m_pConstantBuffer.GetAddressOf()));

	
	// ---------------------------------------------------------------
	// 리소스 로드 
	// ---------------------------------------------------------------
	treeMesh.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/Tree.fbx");
	charMesh.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/Character.fbx");
	zeldaMesh.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/zeldaPosed001.fbx");


	// ---------------------------------------------------------------
	// 샘플러 생성
	// ---------------------------------------------------------------
	D3D11_SAMPLER_DESC sampDesc = {};
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

	m_WorldTree = XMMatrixTranslation(m_TreePos[0], m_TreePos[1], m_TreePos[2]);
	m_WorldChar = XMMatrixTranslation(m_CharPos[0], m_CharPos[1], m_CharPos[2]);
	m_WorldZelda = XMMatrixTranslation(m_ZeldaPos[0], m_ZeldaPos[1], m_ZeldaPos[2]);

	// 카메라(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// 카메라 위치
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라가 바라보는 위치
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라의 위쪽 방향 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// 왼손 좌표계(LH) 기준 

	// 투영행렬(Projection) : 카메라의 시야각(FOV), 화면 종횡비, Near, Far 
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4,m_ClientWidth / (FLOAT)m_ClientHeight, 0.01f, 100.0f);


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
	treeMesh.Clear();
	charMesh.Clear();
	zeldaMesh.Clear();

	// 기본 텍스처 해제
	Material::DestroyDefaultTextures();
}



// [ ImGui ]
bool TestApp::InitImGUI()
{
	// ???? 
	// DXGI Factory 생성 및 첫 번째 Adapter 가져오기 (ImGui)
	// HR_T(CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)m_pDXGIFactory.GetAddressOf()));
	// HR_T(m_pDXGIFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(m_pDXGIAdapter.GetAddressOf())));

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