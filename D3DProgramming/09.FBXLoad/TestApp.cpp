#include "TestApp.h"
#include "../Common/Helper.h"

#include <string> 
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <Directxtk/DDSTextureLoader.h>
#include <windows.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "Psapi.lib")


// [ 상수 버퍼 CB ] 
struct ConstantBuffer
{
	Matrix mWorld;			// 월드 변환 행렬 : 64bytes
	Matrix mView;			// 뷰 변환 행렬   : 64bytes
	Matrix mProjection;		// 투영 변환 행렬 : 64bytes

	Vector4 vLightDir;		// 광원 방향
	Vector4 vLightColor;	// 광원 색상
	Vector4 vOutputColor;	// 출력 색상 

	Vector4 vEyePos;      // 카메라 위치

	Vector4 vAmbient;     // 머티리얼 Ambient
	Vector4 vDiffuse;     // 머티리얼 Diffuse
	Vector4 vSpecular;    // 머티리얼 Specular
	float   fShininess = 40.0f;   // 반짝임 정도
	float   pad[3];       // 16바이트 정렬 패딩
};

TestApp::TestApp() : GameApp()
{

}

TestApp::~TestApp()
{
}

bool TestApp::Initialize()
{
	__super::Initialize();

	// if (!InitD3D())		return false;
	if (!m_D3DDevice.Initialize(m_hWnd, m_ClientWidth, m_ClientHeight)) return false;
	if (!InitScene())	return false;
	if (!InitImGUI())	return false;

	return true;
}

void TestApp::Uninitialize()
{
	UninitImGUI();
	CheckDXGIDebug();	// DirectX 리소스 누수 체크
	m_D3DDevice.Cleanup();
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
		XMConvertToRadians(m_CameraFOV), // degree → radian 변환
		float(m_ClientWidth) / float(m_ClientHeight),
		m_CameraNear,
		m_CameraFar
	);
}     



// ★ [ 렌더링 ] 
void TestApp::Render()
{
	// 0. 그릴 대상 설정 (렌더 타겟 & 뎁스스텐실 설정) 및 화면 초기화 (컬러 + 깊이 버퍼)
	const float clearColor[4] = { m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w };
	m_D3DDevice.BeginFrame(clearColor);


	// 1. 렌더링 파이프라인 스테이지 설정 ( Draw 호출 전에 세팅하고 호출 해야함 )	

	// CB 업데이트
	ConstantBuffer cb;
	cb.mWorld		= XMMatrixTranspose(m_World);
	cb.mView		= XMMatrixTranspose(m_View);
	cb.mProjection	= XMMatrixTranspose(m_Projection);
	cb.vLightDir	= m_LightDirEvaluated;
	cb.vLightColor	= m_LightColor;
	cb.vEyePos		= XMFLOAT4(m_CameraPos[0], m_CameraPos[1], m_CameraPos[2], 1.0f);

	// 머티리얼 + 블린퐁 파라미터
	cb.vAmbient		= m_MaterialAmbient;  
	cb.vDiffuse		= m_LightDiffuse;     
	cb.vSpecular	= m_MaterialSpecular; 
	cb.fShininess	= m_Shininess;


	m_D3DDevice.GetDeviceContext()->IASetInputLayout(m_pInputLayout.Get());
	m_D3DDevice.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// m_pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &m_VertextBufferStride, &m_VertextBufferOffset);
	// m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);

	m_D3DDevice.GetDeviceContext()->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetShader(m_pPixelShader.Get(), nullptr, 0);

	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_D3DDevice.GetDeviceContext()->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());

	//m_pDeviceContext->PSSetShaderResources(0, 1, m_pTexture.GetAddressOf());   // 큐브 텍스처
	//m_pDeviceContext->PSSetShaderResources(1, 1, m_pNormal.GetAddressOf());    // 노멀맵
	//m_pDeviceContext->PSSetShaderResources(2, 1, m_pSpecular.GetAddressOf());  // 스페큘러맵

	m_D3DDevice.GetDeviceContext()->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	// ConstantBuffer 등 설정 후 draw
	treeMesh.Render(m_D3DDevice.GetDeviceContext());
	charMesh.Render(m_D3DDevice.GetDeviceContext());
	zeldaMesh.Render(m_D3DDevice.GetDeviceContext());

	// m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0); // draw


	// 2. UI 그리기 
	Render_ImGui();


	// 3. 스왑체인 교체 (화면 출력 : 백 버퍼 <-> 프론트 버퍼 교체)
	m_D3DDevice.EndFrame(); // m_pSwapChain->Present(0, 0);
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
	ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_Always);

	// [ Cube & Light Control UI ]
	ImGui::Begin("Cube & Light Control");

	ImGui::Text("");

	// [ Cube UI ]
	ImGui::Text("[ Cube ]");
	ImGui::SliderAngle("Cube Yaw (Y axis)", &m_CubeYaw, -180.0f, 180.0f);
	ImGui::SliderAngle("Cube Pitch (X axis)", &m_CubePitch, -180.0f, 180.0f);

	ImGui::Text("");

	// [ Light ]
	ImGui::Text("[ Light ]");
	ImGui::ColorEdit3("Light Color", (float*)&m_LightColor);
	ImGui::SliderFloat3("Light Dir", (float*)&m_InitialLightDir, -1.0f, 1.0f);
	XMVECTOR dir = XMLoadFloat4(&m_InitialLightDir);
	dir = XMVector3Normalize(dir);
	XMStoreFloat4(&m_LightDirEvaluated, dir);

	ImGui::ColorEdit3("Ambient Light (I_a)", (float*)&m_LightAmbient); 
	ImGui::ColorEdit3("Diffuse Light (I_l)", (float*)&m_LightDiffuse);                                                 
	ImGui::SliderFloat("Shininess (alpha)", &m_Shininess, 10.0f, 5000.0f);
	
	ImGui::Text("");

	// [ Material ]
	ImGui::Text("[ Material ]");
	ImGui::ColorEdit3("Ambient (k_a)", (float*)&m_MaterialAmbient);   // 환경광 반사 계수 
																	  // 난반사 계수 -> Diffuse 텍스처 
	ImGui::ColorEdit3("Specular (k_s)", (float*)&m_MaterialSpecular); // 정반사 계수 
	
	ImGui::Text("");

	// [ Camera ]
	ImGui::Text("[ Camera ]");
	ImGui::SliderFloat3("Camera Pos", m_CameraPos, -1000.0f, 1000.0f);

	ImGui::SliderAngle("Camera Yaw (Y axis)", &m_CameraYaw, -180.0f, 180.0f);
	ImGui::SliderAngle("CameraPitch (X axis)", &m_CameraPitch, -90.0f, 90.0f);

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

	// 카메라(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// 카메라 위치
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라가 바라보는 위치
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라의 위쪽 방향 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// 왼손 좌표계(LH) 기준 

	// 투영행렬(Projection) : 카메라의 시야각(FOV), 화면 종횡비, Near, Far 
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4,m_ClientWidth / (FLOAT)m_ClientHeight, 0.01f, 100.0f);


	return true;
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