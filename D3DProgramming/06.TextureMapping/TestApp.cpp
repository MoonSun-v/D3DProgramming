#include "TestApp.h"
#include "../Common/Helper.h"

#include <string> 
#include <dxgi1_3.h>
#include <d3dcompiler.h>

#include <windows.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "Psapi.lib")


// [ 정점 선언 ]
struct Vertex
{
	Vector3 Pos;		// 위치 정보
	Vector3 Normal;		// 정점 법선 (광원 계산용)
};

// [ 상수 버퍼 CB ] 
struct ConstantBuffer
{
	Matrix mWorld;			// 월드 변환 행렬 : 64bytes
	Matrix mView;			// 뷰 변환 행렬   : 64bytes
	Matrix mProjection;		// 투영 변환 행렬 : 64bytes

	Vector4 vLightDir;		// 광원 방향
	Vector4 vLightColor;	// 광원 색상
	Vector4 vOutputColor;	// 출력 색상 (객체 색상 등)
};

TestApp::TestApp() : GameApp()
{

}

TestApp::~TestApp()
{
	UninitScene();
	UninitD3D();
}

bool TestApp::Initialize()
{
	__super::Initialize();

	if (!InitD3D())		return false;
	if (!InitScene())	return false;
	if (!InitImGUI())	return false;
	return true;
}

void TestApp::Uninitialize()
{
	UninitImGUI();
	CheckDXGIDebug();	// DirectX 리소스 누수 체크
}

void TestApp::Update()
{
	__super::Update();

	float totalTime = TimeSystem::m_Instance->TotalTime();

	// [ Cube 월드 행렬 ] -  Y축 기준으로 회전 (Yaw, Pitch 적용)
	XMMATRIX mRotYaw = XMMatrixRotationY(m_CubeYaw);
	XMMATRIX mRotPitch = XMMatrixRotationX(m_CubePitch);
	m_World = mRotPitch * mRotYaw;


	// [ 광원 처리 ]
	m_LightDirEvaluated = m_InitialLightDir; 


	// [ 카메라 갱신 ] 
	m_Camera.m_Position = { m_CameraPos[0], m_CameraPos[1], m_CameraPos[2] }; // 카메라 위치 동기화
	m_Camera.Update(m_Timer.DeltaTime());	// 카메라 상태 업데이트 
	m_Camera.GetViewMatrix(m_View);			// View 행렬 갱신


	// [ 투영 행렬 갱신 ] : 카메라 Projection 행렬 갱신 (FOV, Near, Far 반영)
	float aspect = float(m_ClientWidth) / float(m_ClientHeight);
	m_Projection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(m_CameraFOV), // degree → radian 변환
		aspect,
		m_CameraNear,
		m_CameraFar
	);
}



// ★ [ 렌더링 ] 
void TestApp::Render()
{
	// 0. 그릴 대상 설정 (렌더 타겟 & 뎁스스텐실 설정)
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

	// 1. 화면 초기화 (컬러 + 깊이 버퍼)
	const float clear_color_with_alpha[4] = { m_ClearColor.x , m_ClearColor.y , m_ClearColor.z, m_ClearColor.w };
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), clear_color_with_alpha);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // 뎁스버퍼 1.0f로 초기화.


	// 2. 렌더링 파이프라인 스테이지 설정 ( Draw 호출 전에 해야함 )	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정점을 이어서 그릴 방식 (삼각형 리스트 방식)
	m_pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &m_VertextBufferStride, &m_VertextBufferOffset); // (Stride: 정점 크기, Offset: 시작 위치)
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());
	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_pDeviceContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDeviceContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());


	// 3. 상수 버퍼 업데이트 & 그리기 
	ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose(m_World);
	cb.mView = XMMatrixTranspose(m_View);
	cb.mProjection = XMMatrixTranspose(m_Projection);
	cb.vLightDir = m_LightDirEvaluated; // 단일 라이트 
	cb.vLightColor = m_LightColor;		
	cb.vOutputColor = XMFLOAT4(0, 0, 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);


	// 4. 라이트 표시 (광원 위치를 작은 큐브로 렌더링)
	XMMATRIX mLight = XMMatrixTranslationFromVector(5.0f * XMLoadFloat4(&m_LightDirEvaluated));
	XMMATRIX mLightScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	mLight = mLightScale * mLight;

	// 상수 버퍼 갱신 (광원 위치 + 색상 반영)
	cb.mWorld = XMMatrixTranspose(mLight);
	cb.vOutputColor = m_LightColor;
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// 라이트 전용 픽셀 셰이더 적용 후 큐브 렌더링
	m_pDeviceContext->PSSetShader(m_pPixelShaderSolid.Get(), nullptr, 0);
	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	// 4. UI 그리기 
	Render_ImGui();


	// 5. 스왑체인 교체 (화면 출력 : 백 버퍼 <-> 프론트 버퍼 교체)
	m_pSwapChain->Present(0, 0);
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

	// Cube 회전
	ImGui::Text("Cube Rotation");
	ImGui::SliderAngle("Yaw (Y axis)", &m_CubeYaw, -180.0f, 180.0f);
	ImGui::SliderAngle("Pitch (X axis)", &m_CubePitch, -180.0f, 180.0f);

	ImGui::Text("");

	// [ Light UI ]
	ImGui::Text("[ Light ]");

	// Light 색상
	ImGui::Text("Light Color");
	ImGui::ColorEdit3("Light Color", (float*)&m_LightColor);

	// Light 방향
	ImGui::Text("Light Direction");
	ImGui::SliderFloat3("Light Dir", (float*)&m_InitialLightDir, -1.0f, 1.0f);

	ImGui::Text("");

	// [ Camera UI ]
	ImGui::Text("[ Camera ]");
	 
	// 카메라 월드 위치 (x,y,z)
	ImGui::Text("1. Camera Pos");
	ImGui::SliderFloat3("Camera Pos", m_CameraPos, -100.0f, 100.0f);

	// 카메라 FOV (degree)
	ImGui::Text("2. FOV (deg)");
	ImGui::SliderFloat("FOV (deg)", &m_CameraFOV, 10.0f, 120.0f);

	// 카메라 Near, Far
	ImGui::Text("3. Near");
	ImGui::SliderFloat("Near", &m_CameraNear, 0.01f, 10.0f);

	ImGui::Text("4. Far");
	ImGui::SliderFloat("Far", &m_CameraFar, 10.0f, 5000.0f);


	// [ 끝 ] 
	ImGui::End();

	// [ ImGui 프레임 최종 렌더링 ]
	ImGui::Render();  // ImGui 내부에서 렌더링 데이터를 준비
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // DX11로 실제 화면에 출력
}


// ★ [ Direct3D11 초기화 ] 
bool TestApp::InitD3D()
{
	
	HRESULT hr = 0; // 결과값 : DirectX 함수는 HRESULT 반환


	// =====================================
	// 0. DXGI Factory 생성 및 첫 번째 Adapter 가져오기 (ImGui)
	// =====================================
	HR_T(CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)m_pDXGIFactory.GetAddressOf()));
	HR_T(m_pDXGIFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(m_pDXGIAdapter.GetAddressOf()))); 



	// =====================================
	// 1. 디바이스 / 디바이스 컨텍스트 생성
	// =====================================

	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG; // 디버그 레이어 활성화 (검증 메시지 출력)
#endif
	// 그래픽 카드 하드웨어의 스펙으로 호환되는 가장 높은 DirectX 기능레벨로 생성하여 드라이버가 작동 한다.
	// 인터페이스는 Direc3D11 이지만 GPU드라이버는 D3D12 드라이버가 작동할수도 있다.
	D3D_FEATURE_LEVEL featureLevels[] = // index 0부터 순서대로 시도한다.
	{ 
			D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0
	};
	D3D_FEATURE_LEVEL actualFeatureLevel; // 최종 피처 레벨을 저장할 변수

	HR_T(D3D11CreateDevice(
		nullptr,						// 기본 어댑터 사용
		D3D_DRIVER_TYPE_HARDWARE,		// 하드웨어 렌더링 사용
		0,								// 소프트웨어 드라이버 없음
		creationFlags,					// 디버그 플래그
		featureLevels,					// Feature Level 설정 
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,				// SDK 버전
		m_pDevice.GetAddressOf(),		// 디바이스 반환
		&actualFeatureLevel,			// Feature Level 반환
		m_pDeviceContext.GetAddressOf() // 디바이스 컨택스트 반환 
	));




	// =====================================
	// 2. DXGI Factory 생성 및 스왑체인 준비
	// =====================================

	UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ComPtr<IDXGIFactory2> pFactory;
	HR_T(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(pFactory.GetAddressOf())));

	// 스왑체인(백버퍼) 설정 
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;									// 백버퍼 개수 
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Width = m_ClientWidth;
	swapChainDesc.Height = m_ClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// 백버퍼 포맷: 32비트 RGBA
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// 백버퍼의 용도: 렌더타겟 출력
	swapChainDesc.SampleDesc.Count = 1;								// 멀티샘플링(안티에일리어싱) 사용 안함 (기본: 1, 안 씀)
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;				// Recommended for flip models
	swapChainDesc.Stereo = FALSE;									// 3D 비활성화
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// 전체 화면 전환 허용
	swapChainDesc.Scaling = DXGI_SCALING_NONE;						// 창의 크기와 백 버퍼의 크기가 다를 때. 자동 스케일링X 

	HR_T(pFactory->CreateSwapChainForHwnd(
		m_pDevice.Get(),
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		m_pSwapChain.GetAddressOf()
	));




	// =====================================
    // 3. 렌더타겟뷰(Render Target View) 생성
    // =====================================
	
	ComPtr<ID3D11Texture2D> pBackBufferTexture; 
	HR_T(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(pBackBufferTexture.GetAddressOf())));		// 스왑체인의 0번 버퍼(백버퍼) 가져옴
	HR_T(m_pDevice->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, m_pRenderTargetView.GetAddressOf())); // 백버퍼 텍스처를 기반으로 렌더타겟뷰 생성
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), NULL); // ?



	// =====================================
	// 4. 뷰포트(Viewport) 설정
	// =====================================

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;                        // 뷰포트 시작 X
	viewport.TopLeftY = 0;                        // 뷰포트 시작 Y
	viewport.Width = (float)m_ClientWidth;        // 뷰포트 너비
	viewport.Height = (float)m_ClientHeight;      // 뷰포트 높이
	viewport.MinDepth = 0.0f;                     // 깊이 버퍼 최소값
	viewport.MaxDepth = 1.0f;                     // 깊이 버퍼 최대값

	// 래스터라이저(RS: Rasterizer) 단계에 뷰포트 설정
	m_pDeviceContext->RSSetViewports(1, &viewport);




	// =====================================
	// 5. 깊이&스텐실 버퍼 및 뷰 생성
	// =====================================

	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = m_ClientWidth;
	descDepth.Height = m_ClientHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24비트 깊이 + 8비트 스텐실
	descDepth.SampleDesc.Count = 1;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ComPtr<ID3D11Texture2D> pTextureDepthStencil;
	HR_T(m_pDevice->CreateTexture2D(&descDepth, nullptr, pTextureDepthStencil.GetAddressOf()));

	// 깊이 스텐실 뷰 
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	HR_T(m_pDevice->CreateDepthStencilView(pTextureDepthStencil.Get(), &descDSV, m_pDepthStencilView.GetAddressOf()));

	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get()); // ?

	return true;
}

void TestApp::UninitD3D()
{
}



// ★ [ Scene 초기화 ] 
bool TestApp::InitScene()
{

	HRESULT hr = 0;                       // DirectX 함수 결과값
	ID3D10Blob* errorMessage = nullptr;   // 셰이더 컴파일 에러 메시지 버퍼	


	// ================================================================
	// 1. 정점(Vertex) 데이터 준비 및 정점 버퍼 생성
	// ================================================================
	
	// 큐브 모델의 각 면에 대한 정점 데이터 (위치 + 노멀벡터)
	// Local Space(모델 좌표계) 기준
	Vertex vertices[] =
	{
		// 윗면 (Normal Y+)
		{ Vector3(-1.0f, 1.0f, -1.0f),	Vector3(0.0f, 1.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),	Vector3(0.0f, 1.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),	Vector3(0.0f, 1.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),	Vector3(0.0f, 1.0f, 0.0f) },

		// 아랫면 (Normal Y-)
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, -1.0f, 0.0f) },	
		{ Vector3(1.0f, -1.0f, -1.0f),	Vector3(0.0f, -1.0f, 0.0f) },
		{ Vector3(1.0f, -1.0f, 1.0f),	Vector3(0.0f, -1.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, 1.0f),	Vector3(0.0f, -1.0f, 0.0f) },

		// 왼쪽면 (Normal X-)
		{ Vector3(-1.0f, -1.0f, 1.0f),	Vector3(-1.0f, 0.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, -1.0f),	Vector3(-1.0f, 0.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),	Vector3(-1.0f, 0.0f, 0.0f) },

		// 오른쪽면 (Normal X+)
		{ Vector3(1.0f, -1.0f, 1.0f),	Vector3(1.0f, 0.0f, 0.0f) },
		{ Vector3(1.0f, -1.0f, -1.0f),	Vector3(1.0f, 0.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),	Vector3(1.0f, 0.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),	Vector3(1.0f, 0.0f, 0.0f) },

		// 뒷면 (Normal Z-)
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f) }, 
		{ Vector3(1.0f, -1.0f, -1.0f),	Vector3(0.0f, 0.0f, -1.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),	Vector3(0.0f, 0.0f, -1.0f) },
		{ Vector3(-1.0f, 1.0f, -1.0f),	Vector3(0.0f, 0.0f, -1.0f) },

		// 앞면 (Normal Z+)
		{ Vector3(-1.0f, -1.0f, 1.0f),	Vector3(0.0f, 0.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, 1.0f),	Vector3(0.0f, 0.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),	Vector3(0.0f, 0.0f, 1.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),	Vector3(0.0f, 0.0f, 1.0f) },
	};
	m_VertexCount = ARRAYSIZE(vertices); // 정점 개수 저장


	// 정점 버퍼 속성 정의 
	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = sizeof(Vertex) * m_VertexCount;    // 버퍼 크기 (정점 크기 × 정점 개수)
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;          // 용도: 정점 버퍼
	bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU가 읽고 쓰는 기본 버퍼
	bd.CPUAccessFlags = 0;

	// 초기 데이터 전달용 구조체
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices; // 정점 배열 데이터 할당 

	// 정점 버퍼 생성
	HR_T(m_pDevice->CreateBuffer(&bd, &vbData, m_pVertexBuffer.GetAddressOf()));

	// 버텍스 버퍼 정보
	m_VertextBufferStride = sizeof(Vertex); // 정점 하나의 크기
	m_VertextBufferOffset = 0;              // 시작 오프셋 (0)



	// ================================================================
	// 2. 인덱스 버퍼 생성
	// ================================================================

	WORD indices[] =
	{
		// 윗면, 아랫면, 왼쪽, 오른쪽, 뒤, 앞
		3,1,0, 2,1,3,
		6,4,5, 7,4,6,
		11,9,8, 10,9,11,
		14,12,13, 15,12,14,
		19,17,16, 18,17,19,
		22,20,21, 23,20,22
	};
	m_nIndices = ARRAYSIZE(indices);	// 인덱스 개수 저장

	// 인덱스 버퍼 속성 정의
	bd = {};
	bd.ByteWidth = sizeof(WORD) * m_nIndices;		  // 전체 인덱스 배열 크기
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;           // 인덱스 버퍼로 사용
	bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU에서 읽기 전용(수정X)
	bd.CPUAccessFlags = 0;

	// 초기 데이터 전달
	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	// 인덱스 버퍼 생성
	HR_T(m_pDevice->CreateBuffer(&bd, &ibData, m_pIndexBuffer.GetAddressOf()));



	// ================================================================
	// 3. 버텍스 셰이더(Vertex Shader) 컴파일 및 생성
	// ================================================================

	ComPtr<ID3DBlob> vertexShaderBuffer; // 컴파일된 버텍스 셰이더 코드(hlsl) 저장 버퍼
	
	// ' HLSL 파일에서 main 함수를 vs_4_0 규격으로 컴파일 '
	HR_T(CompileShaderFromFile(L"../Shaders/06.BasicVertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));



	// ================================================================
	// 4. 입력 레이아웃(Input Layout) 생성
	// ================================================================

	// 정점 셰이더가 입력으로 받을 데이터 형식 정의
	// { SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate }

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // POSITION : float3 (12바이트)
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },  // NORMAL   : float3 (12바이트, 오프셋 12)
	};

	// 버텍스 셰이더의 Input시그니처와 비교해 유효성 검사 후 -> InputLayout 생성
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));



	// ================================================================
	// 5. 픽셀 셰이더(Pixel Shader) 컴파일 및 생성
	// ================================================================

	ComPtr<ID3DBlob> pixelShaderBuffer; // 컴파일된 버텍스 픽셀 코드(hlsl) 저장 버퍼

	// ' HLSL 파일에서 main 함수를 ps_4_0 규격으로 컴파일 '
	HR_T(CompileShaderFromFile(L"../Shaders/06.BasicPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));

	// 빛(라이트) 위치를 시각화할 별도 픽셀 셰이더
	HR_T(CompileShaderFromFile(L"../Shaders/06.SolidPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShaderSolid.GetAddressOf()));



	// ================================================================
	// 6.  상수 버퍼(Constant Buffer) 생성
	// ================================================================

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer); // World, View, Projection + Light
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bd, nullptr, m_pConstantBuffer.GetAddressOf()));



	// ================================================================
	// 7. 행렬(World, View, Projection) 설정
	// ================================================================
	
	m_World = XMMatrixIdentity(); // 단위 행렬 

	// 카메라(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// 카메라 위치
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라가 바라보는 위치
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// 카메라의 위쪽 방향 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// 왼손 좌표계(LH) 기준 

	// 투영행렬(Projection)
	m_Projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,                             // FOV( 90도: 세로 시야각 ) 
		m_ClientWidth / (FLOAT)m_ClientHeight, // 화면 종횡비
		0.01f,                                 // Near
		100.0f                                 // Far 
	);


	return true;
}

void TestApp::UninitScene()
{
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
	ImGui_ImplDX11_Init(this->m_pDevice.Get(), this->m_pDeviceContext.Get());	// DirectX11 렌더러용 초기화


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