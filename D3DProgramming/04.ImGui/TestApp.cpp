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
	Vector3 position;	// 위치 정보
	Vector4 color;		// 색상 정보

	Vertex(float x, float y, float z) : position(x, y, z) {}
	Vertex(Vector3 position) : position(position) {}
	Vertex(Vector3 position, Vector4 color): position(position), color(color) {}
};

// [ 상수 버퍼 CB ]
struct ConstantBuffer
{
	Matrix mWorld;       // 월드 변환 행렬
	Matrix mView;        // 뷰 변환 행렬
	Matrix mProjection;  // 투영 변환 행렬
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


	// [ 1번째 Cube ] : 단순히 Y축 회전 
	m_World1 = XMMatrixRotationY(totalTime);


	// [ 2번째 Cube ] : 궤도 회전 + 제자리 회전 + 스케일 축소 + 이동
	// mOrbit : 다른 축을 기준으로 회전 
	XMMATRIX spin2 = XMMatrixRotationZ(-totalTime);					// 제자리 Z축 회전
	XMMATRIX orbit2 = XMMatrixRotationY(-totalTime * 1.5f);			// 중심을 기준으로 Y축 궤도 회전
	XMMATRIX translate2 = XMMatrixTranslation(-4.0f, 0.0f, 0.0f);	// 왼쪽으로 이동
	XMMATRIX scale2 = XMMatrixScaling(0.3f, 0.3f, 0.3f);			// 크기 축소

	// scale -> spin -> translate -> orbit 
	XMMATRIX temp2_1 = XMMatrixMultiply(scale2, spin2);
	XMMATRIX temp2_2 = XMMatrixMultiply(XMMatrixMultiply(scale2, spin2), translate2);
	m_World2 = XMMatrixMultiply(temp2_2, orbit2);					// 최종 월드 행렬


	// [ 3번째 Cube ] : 2번 큐브의 자식
	XMMATRIX spin3 = XMMatrixRotationZ(totalTime);					// 제자리 Z축 회전
	XMMATRIX orbit3 = XMMatrixRotationY(totalTime * 3.0f);			// 2번 큐브 중심을 기준으로 궤도 회전
	XMMATRIX translate3 = XMMatrixTranslation(-2.0f, 0.0f, 0.0f);	// 왼쪽으로 이동
	XMMATRIX scale3 = XMMatrixScaling(0.5f, 0.5f, 0.5f);			// 크기 축소

	// scale -> spin -> translate -> orbit -> orbit 
	XMMATRIX temp3_1 = XMMatrixMultiply(scale3, spin3);
	XMMATRIX temp3_2 = XMMatrixMultiply(temp3_1, translate3);
	XMMATRIX local3 = XMMatrixMultiply(temp3_2, orbit3);

	m_World3 = XMMatrixMultiply(local3, m_World2);	// 2번 큐브의 자식으로 연결
}



// ★ [ 렌더링 ] 
void TestApp::Render()
{
	// 0. 그릴 대상 설정
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

	// 1. 화면 지우기 
	const float clear_color_with_alpha[4] = { m_ClearColor.x , m_ClearColor.y , m_ClearColor.z, m_ClearColor.w };
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), clear_color_with_alpha);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // 뎁스버퍼 1.0f로 초기화.


	// 2. 렌더링 파이프라인 설정
	// ( Draw계열 함수 호출 전 -> 렌더링 파이프라인에 필수 스테이지 설정 해야함 )	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정점을 이어서 그릴 방식 (삼각형 리스트 방식)
	m_pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &m_VertextBufferStride, &m_VertextBufferOffset); // (Stride: 정점 크기, Offset: 시작 위치)
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());
	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_pDeviceContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());


	// 3. 그리기 (인덱스 버퍼 기반)
	// ※ 인덱스 버퍼 없이 정점만 사용할 경우 -> Draw() 사용

	ConstantBuffer cb1;
	cb1.mWorld = XMMatrixTranspose(m_World1);
	cb1.mView = XMMatrixTranspose(m_View);
	cb1.mProjection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb1, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	ConstantBuffer cb2;
	cb2.mWorld = XMMatrixTranspose(m_World2);
	cb2.mView = XMMatrixTranspose(m_View);
	cb2.mProjection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb2, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	ConstantBuffer cb3;
	cb3.mWorld = XMMatrixTranspose(m_World3);
	cb3.mView = XMMatrixTranspose(m_View);
	cb3.mProjection = XMMatrixTranspose(m_Projection);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb3, 0, 0);

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


	// 1. [ 새로운 ImGui 프레임 시작 ]
	ImGui_ImplDX11_NewFrame();		// DirectX11 렌더러용 프레임 준비
	ImGui_ImplWin32_NewFrame();		// Win32 플랫폼용 프레임 준비
	ImGui::NewFrame();				// ImGui 내부 프레임 초기화



	// 2. [ ImGui Demo Window 표시 (옵션) ]
	if (m_show_demo_window) ImGui::ShowDemoWindow(&m_show_demo_window);
	// ImGui가 제공하는 예제 윈도우 표시
	// &m_show_demo_window -> 체크박스를 통해 창을 닫을 수 있음



	// 3. [ 사용자 정의 윈도우 생성 ]
	ImGui::Begin("Hello, world!");								// "Hello, world!" 이름의 새로운 창 생성

	ImGui::Text("This is some useful text.");					// 단순 텍스트 출력
	ImGui::Checkbox("Demo Window", &m_show_demo_window);		// 체크박스 생성: Demo Window ON/OFF
	ImGui::Checkbox("Another Window", &m_show_another_window);

	ImGui::SliderFloat("float", &m_f, 0.0f, 1.0f);				// 슬라이더 생성: m_f 값을 0.0 ~ 1.0 범위에서 조절

	if (ImGui::Button("Button")) m_counter++;					// 버튼 생성: 클릭 시 m_counter 증가

	ImGui::SameLine();
	ImGui::Text("counter = %d", m_counter);						// 버튼 옆에 같은 라인으로 텍스트 표시

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate); // 프레임 속도 표시

	// GPU 메모리 정보 표시
	std::string str;
	ImGui::Text("\nDisplay Memory");
	GetDisplayMemoryInfo(str);
	ImGui::Text("%s", str.c_str());

	// 프로세스 메모리 정보 표시
	ImGui::Text("\nProcess Memory");
	GetVirtualMemoryInfo(str);
	ImGui::Text("%s", str.c_str());

	// 색상 편집 위젯 (RGB 슬라이더)
	ImGui::ColorEdit3("clear color", (float*)&m_ClearColor);  // float 배열 주소를 전달하여 3개의 float 값(RGB)을 편집 가능
	
	// 창 닫기
	ImGui::End();



	// 4. [ 다른 간단한 창 표시 ]
	if (m_show_another_window)
	{
		ImGui::Begin("Another Window", &m_show_another_window);			// "Another Window" 생성, &m_show_another_window로 창 닫기 가능
		ImGui::Text("Hello from another window!");						// 텍스트 출력
		if (ImGui::Button("Close Me")) m_show_another_window = false;	// 버튼 클릭 시 창 닫기
		ImGui::End();
	}


	// 5. [ ImGui 프레임 최종 렌더링 ]
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
	HR_T(m_pDevice->CreateDepthStencilView(pTextureDepthStencil.Get(), &descDSV, &m_pDepthStencilView));


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

	Vertex vertices[] =
	{
		{ Vector3(-1.0f, 1.0f, -1.0f),	Vector4(0.0f, 0.0f, 1.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),	Vector4(0.0f, 1.0f, 0.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),	Vector4(0.0f, 1.0f, 1.0f, 1.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),	Vector4(1.0f, 0.0f, 0.0f, 1.0f) },
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector4(1.0f, 0.0f, 1.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, -1.0f),	Vector4(1.0f, 1.0f, 0.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, 1.0f),	Vector4(1.0f, 1.0f, 1.0f, 1.0f) },
		{ Vector3(-1.0f, -1.0f, 1.0f),	Vector4(0.0f, 0.0f, 0.0f, 1.0f) },
	};
	m_VertexCount = ARRAYSIZE(vertices);                  // 정점 개수 저장 


	// 정점 버퍼 속성 구조체
	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = sizeof(Vertex) * m_VertexCount;    // 버퍼 크기 (정점 크기 × 정점 개수)
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;          // 정점 버퍼 용도
	bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU가 읽고 쓰는 기본 버퍼
	bd.CPUAccessFlags = 0;

	// 버퍼에 초기 데이터 복사할 구조체
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices; // 배열 데이터 할당 

	// 정점 버퍼 생성
	HR_T(m_pDevice->CreateBuffer(&bd, &vbData, m_pVertexBuffer.GetAddressOf()));

	// 버텍스 버퍼 정보
	m_VertextBufferStride = sizeof(Vertex); // 정점 하나의 크기
	m_VertextBufferOffset = 0;              // 시작 오프셋 (0)




	// ================================================================
	// 2. 인덱스 버퍼 생성
	// ================================================================
	//
	// 인덱스(Index) 버퍼
	// - 인덱스를 참조해서 정점(Vertex) 재사용 

	WORD indices[] =
	{
		3,1,0, 2,1,3,
		0,5,4, 1,5,0,
		3,4,7, 0,4,3,
		1,6,5, 2,6,1,
		2,7,6, 3,7,2,
		6,4,5, 7,4,6,
	};
	m_nIndices = ARRAYSIZE(indices);	// 인덱스 개수 저장

	// 인덱스 버퍼 속성 정의
	bd = {};
	bd.ByteWidth = sizeof(WORD) * m_nIndices;		  // 전체 인덱스 배열 크기
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;           // 인덱스 버퍼로 사용
	bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU에서 읽기 전용(수정X)
	bd.CPUAccessFlags = 0;

	// 인덱스 데이터 초기화 정보
	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	// 인덱스 버퍼 생성
	HR_T(m_pDevice->CreateBuffer(&bd, &ibData, m_pIndexBuffer.GetAddressOf()));





	// ================================================================
	// 3. 버텍스 셰이더(Vertex Shader) 컴파일 및 생성
	// ================================================================

	ComPtr<ID3DBlob> vertexShaderBuffer; // 컴파일된 버텍스 셰이더 코드(hlsl) 저장 버퍼
	
	// ' HLSL 파일에서 main 함수를 vs_4_0 규격으로 컴파일 '
	HR_T(CompileShaderFromFile(L"TransformVertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));

	// 버텍스 셰이더 객체 생성
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, m_pVertexShader.GetAddressOf()));



	// ================================================================
	// 4. 입력 레이아웃(Input Layout) 생성
	// ================================================================

	// 정점 셰이더가 입력으로 받을 데이터 형식 정의
	// { SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate }

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// 버텍스 셰이더의 Input시그니처와 비교해 유효성 검사 후 -> InputLayout 생성
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));



	// ================================================================
	// 5. 픽셀 셰이더(Pixel Shader) 컴파일 및 생성
	// ================================================================

	ComPtr<ID3DBlob> pixelShaderBuffer; // 컴파일된 버텍스 픽셀 코드(hlsl) 저장 버퍼

	// ' HLSL 파일에서 main 함수를 ps_4_0 규격으로 컴파일 '
	HR_T(CompileShaderFromFile(L"TransformPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));

	// 픽셀 셰이더 객체 생성
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf()));



	// ================================================================
	// 6.  Render() 에서 파이프라인에 바인딩할 상수 버퍼 생성
	// ================================================================

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer));



	// [ 셰이더에 전달할 데이터 설정 ]
	
	// World Matrix 
	m_World1 = XMMatrixIdentity();
	m_World2 = XMMatrixIdentity();

	// View Matrix 
	XMVECTOR Eye = XMVectorSet(0.0f, 1.0f, -5.0f, 0.0f);
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	m_View = XMMatrixLookAtLH(Eye, At, Up);

	// Projection Matrix
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV2, m_ClientWidth / (FLOAT)m_ClientHeight, 0.01f, 100.0f);



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


// ============================================================
// [ GPU 메모리 정보 조회 ]
// ============================================================
void TestApp::GetDisplayMemoryInfo(std::string& out)
{
	DXGI_ADAPTER_DESC desc;
	m_pDXGIAdapter->GetDesc(&desc);

	DXGI_QUERY_VIDEO_MEMORY_INFO local, nonLocal;
	m_pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local);
	m_pDXGIAdapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocal);

	out = std::to_string((desc.DedicatedVideoMemory + desc.SharedSystemMemory) / 1024 / 1024) + " MB\n";
	out += "Dedicated Video Memory : " + std::to_string(desc.DedicatedVideoMemory / 1024 / 1024) + " MB\n";
	out += "Shared System Memory : " + std::to_string(desc.SharedSystemMemory / 1024 / 1024) + " MB\n";

	out += "Local Video Memory: ";
	out += std::to_string(local.Budget / 1024 / 1024) + "MB" + " / " + std::to_string(local.CurrentUsage / 1024 / 1024) + " MB\n";
	out += "NonLocal Video Memory: ";
	out += std::to_string(nonLocal.Budget / 1024 / 1024) + "MB" + " / " + std::to_string(nonLocal.CurrentUsage / 1024 / 1024) + " MB";
}


// ============================================================
// [ 프로세스(가상) 메모리 정보 조회 ]
// ============================================================
void TestApp::GetVirtualMemoryInfo(std::string& out)
{
	HANDLE hProcess = GetCurrentProcess();
	PROCESS_MEMORY_COUNTERS_EX pmc;
	pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);
	GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
	out = "PrivateUsage: " + std::to_string((pmc.PrivateUsage) / 1024 / 1024) + " MB\n";
	out += "WorkingSetSize: " + std::to_string((pmc.WorkingSetSize) / 1024 / 1024) + " MB\n";
	out += "PagefileUsage: " + std::to_string((pmc.PagefileUsage) / 1024 / 1024) + " MB";
}



extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK TestApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return __super::WndProc(hWnd, message, wParam, lParam);
}