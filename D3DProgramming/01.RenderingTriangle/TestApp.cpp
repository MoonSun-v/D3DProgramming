#include "TestApp.h"
#include "../Common/Helper.h"

#include <directxtk/simplemath.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX::SimpleMath;


// [ 정점 선언 ]
struct Vertex
{
	Vector3 position; // 위치 정보
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

	return true;
}

void TestApp::Update()
{

}



// ★ [ 렌더링 ] 과정 : (초기화 → 파이프라인 설정 → 그리기 → 스왑체인 교체) 
void TestApp::Render()
{
	// 1. 화면 칠하기
	float color[4] = { 0.80f, 0.92f, 1.0f, 1.0f }; //  Light Sky Blue 
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);


	// 2. 렌더링 파이프라인 설정
	// ( Draw계열 함수 호출 전 -> 렌더링 파이프라인에 필수 스테이지 설정 해야함 )	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // 정점을 이어서 그릴 방식 (삼각형 리스트 방식)
	m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &m_VertextBufferStride, &m_VertextBufferOffset); // (Stride: 정점 크기, Offset: 시작 위치)
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);	// 입력 레이아웃 설정 
	m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);		
	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);


	// 3. 그리기 
	m_pDeviceContext->Draw(m_VertexCount, 0);


	// 4. 스왑체인 교체 (화면 출력 : 백 버퍼 <-> 프론트 버퍼 교체)
	m_pSwapChain->Present(0, 0);
}



// ★ [ Direct3D11 초기화 ] : (스왑체인 생성 -> 렌더타겟뷰 생성 -> 뷰포트 설정)
bool TestApp::InitD3D()
{
	
	HRESULT hr = 0; // 결과값 : DirectX 함수는 HRESULT 반환



	// =====================================
	// 1. 스왑체인(Swap Chain) 설정
	// =====================================

	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferCount = 1;                                    // 백버퍼 개수 (더블 버퍼링: 1)
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // 백버퍼의 용도: 렌더타겟 출력
	swapDesc.OutputWindow = m_hWnd;                              // 출력할 윈도우 핸들
	swapDesc.Windowed = true;                                    // 창 모드(true) / 전체화면(false)
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // 백버퍼 포맷: 32비트 RGBA

	// 백버퍼(텍스처) 해상도 지정
	swapDesc.BufferDesc.Width = m_ClientWidth;
	swapDesc.BufferDesc.Height = m_ClientHeight;

	// 화면 주사율(Refresh Rate) 지정 (60Hz)
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;

	// 멀티샘플링(안티에일리어싱) 설정 (기본: 1, 안 씀)
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;




	// =====================================
	// 2. 디바이스 / 스왑체인 / 디바이스 컨텍스트 생성
	// =====================================

	UINT creationFlags = 0;

#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG; // 디버그 레이어 활성화 (DirectX API 호출 시 검증 메시지 출력)
#endif

	HR_T(D3D11CreateDeviceAndSwapChain(
		NULL,                           // 기본 어댑터 사용
		D3D_DRIVER_TYPE_HARDWARE,       // 하드웨어 렌더링 사용
		NULL,                           // 소프트웨어 드라이버 없음
		creationFlags,                  // 디버그 플래그
		NULL, NULL,                     // 기본 Feature Level 사용
		D3D11_SDK_VERSION,              // SDK 버전
		&swapDesc,                      // 스왑체인 설명 구조체
		&m_pSwapChain,                  // 스왑체인 반환
		&m_pDevice,                     // 디바이스 반환
		NULL,                           // Feature Level 반환 (사용 안 함)
		&m_pDeviceContext));            // 디바이스 컨텍스트 반환




	// =====================================
    // 3. 렌더타겟뷰(Render Target View) 생성
    // =====================================
	
    // 스왑체인의 0번 버퍼(백버퍼)를 가져옴
	ID3D11Texture2D* pBackBufferTexture = nullptr;
	HR_T(m_pSwapChain->GetBuffer(
		0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));

	// 백버퍼 텍스처를 기반으로 렌더타겟뷰 생성
	HR_T(m_pDevice->CreateRenderTargetView(
		pBackBufferTexture, NULL, &m_pRenderTargetView));  // 텍스처는 내부 참조 증가

	// 텍스처 참조 해제 (렌더타겟뷰가 참조 잡고 있으므로 여기선 해제 가능) // 외부 참조 카운트를 감소시킨다.
	SAFE_RELEASE(pBackBufferTexture);	

	// 출력 병합(OM: Output Merger) 단계에 렌더타겟 바인딩
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);




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



	return true;
}

void TestApp::UninitD3D()
{
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pDevice);
}



// ★ [ Scene 초기화 ] : (Vertex 버퍼 생성 -> Vertex 셰이더 컴파일 및 생성 -> 인풋 레이아웃 설정 -> Pixel 셰이더 컴파일 및 생성)
bool TestApp::InitScene()
{

	HRESULT hr = 0;                       // DirectX 함수 결과값
	ID3D10Blob* errorMessage = nullptr;   // 셰이더 컴파일 에러 메시지 버퍼	


	// ================================================================
	// 1. 정점(Vertex) 데이터 준비 및 정점 버퍼 생성
	// ================================================================
	// 
	// 현재는 월드/뷰/프로젝션 변환을 쓰지 않고,
	// 직접 NDC(Normalized Device Coordinate, -1 ~ +1 좌표계)에 맞게 작성
	// 
	//  - 화면 중앙이 (0,0,0), 왼쪽 위는 (-1,1,0), 오른쪽 아래는 (1,-1,0)
	//  - 삼각형 하나를 구성하기 위해 v0, v1, v2 정의
	// 
	//      /---------------------(1,1,1)   z값은 깊이값
	//     /                      / |   
	// (-1,1,0)----------------(1,1,0)        
	//   |         v1           |   |
	//   |        /   `         |   |       중앙이 (0,0,0)  
	//   |       /  +   `       |   |
	//   |     /         `      |   |
	//	 |   v0-----------v2    |  /
	// (-1,-1,0)-------------(1,-1,0)

	Vertex vertices[] =
	{
		Vector3(-0.5, -0.5, 0.5), // v0: 왼쪽 아래
		Vector3(0.0,  0.5, 0.5), // v1: 위쪽 중앙
		Vector3(0.5, -0.5, 0.5), // v2: 오른쪽 아래	
	};


	// 정점 버퍼 속성 구조체
	D3D11_BUFFER_DESC vbDesc = {};
	m_VertexCount = ARRAYSIZE(vertices);                  // 정점 개수
	vbDesc.ByteWidth = sizeof(Vertex) * m_VertexCount;    // 버퍼 크기 (정점 크기 × 정점 개수)
	vbDesc.CPUAccessFlags = 0;                            // CPU 접근X
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;          // 정점 버퍼 용도
	vbDesc.MiscFlags = 0;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;                   // GPU가 읽고 쓰는 기본 버퍼

	// 버퍼에 초기 데이터 복사할 구조체
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices; // 버텍스 배열 주소

	// 정점 버퍼 생성
	HR_T(hr = m_pDevice->CreateBuffer(&vbDesc, &vbData, &m_pVertexBuffer));

	// 버텍스 버퍼 정보
	m_VertextBufferStride = sizeof(Vertex); // 정점 하나의 크기
	m_VertextBufferOffset = 0;              // 시작 오프셋 (0)




	// ================================================================
	// 2. 버텍스 셰이더(Vertex Shader) 컴파일 및 생성
	// ================================================================

	ID3DBlob* vertexShaderBuffer = nullptr; // 컴파일된 버텍스 셰이더 코드(hlsl) 저장 버퍼

	// ' HLSL 파일에서 main 함수를 vs_4_0 규격으로 컴파일 '
	HR_T(CompileShaderFromFile(L"BasicVertexShader.hlsl", "main", "vs_4_0", &vertexShaderBuffer));

	// 버텍스 셰이더 객체 생성
	HR_T(m_pDevice->CreateVertexShader(
		vertexShaderBuffer->GetBufferPointer(), // 필요한 데이터를 복사하며 객체 생성 
		vertexShaderBuffer->GetBufferSize(), NULL, &m_pVertexShader));




	// ================================================================
	// 3. 입력 레이아웃(Input Layout) 생성
	// ================================================================

	// 정점 셰이더가 입력으로 받을 데이터 형식 정의
	// { SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate }

	D3D11_INPUT_ELEMENT_DESC layout[] =  
	{	
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 } // ' POSITION: float3 (R32G32B32_FLOAT), 슬롯 0번, 정점 당 데이터 '
	};

	// 버텍스 셰이더의 Input시그니처와 비교해 유효성 검사 후 -> InputLayout 생성
	HR_T(hr = m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
		vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_pInputLayout));

	// 복사했으니 버퍼는 해제 가능
	SAFE_RELEASE(vertexShaderBuffer); 




	// ================================================================
	// 4. 픽셀 셰이더(Pixel Shader) 컴파일 및 생성
	// ================================================================

	ID3DBlob* pixelShaderBuffer = nullptr; // 컴파일된 버텍스 픽셀 코드(hlsl) 저장 버퍼

	// ' HLSL 파일에서 main 함수를 ps_4_0 규격으로 컴파일 '
	HR_T(CompileShaderFromFile(L"BasicPixelShader.hlsl", "main", "ps_4_0", &pixelShaderBuffer));

	// 픽셀 셰이더 객체 생성
	HR_T(m_pDevice->CreatePixelShader(	  // 필요한 데이터를 복사하며 객체 생성 
		pixelShaderBuffer->GetBufferPointer(),
		pixelShaderBuffer->GetBufferSize(), NULL, &m_pPixelShader));

	// 복사했으니 버퍼는 해제 가능
	SAFE_RELEASE(pixelShaderBuffer);



	return true;
}

void TestApp::UninitScene()
{
	SAFE_RELEASE(m_pVertexBuffer);
	SAFE_RELEASE(m_pInputLayout);
	SAFE_RELEASE(m_pVertexShader);
	SAFE_RELEASE(m_pPixelShader);
}
