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

	m_Camera.m_Position = Vector3(0.0f, 0.0f, -500.0f);
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

	boxHuman.Update(deltaTime); // 본 애니메이션 업데이트
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

	// Rigid용 본 행렬 버퍼 준비
	BoneMatrixContainer boneCB;
	boneCB.Clear();
	// boneCB.SetMatrix(0, XMMatrixTranspose(m_WorldChar)); =
    // boxHuman은 단일 본
	if (!boxHuman.m_Skeleton.empty())
		boneCB.SetMatrix(0, XMMatrixTranspose(boxHuman.m_Skeleton[0].m_Model * boxHuman.m_World));

	// GPU 상수 버퍼 업데이트
	m_D3DDevice.GetDeviceContext()->UpdateSubresource(m_pBoneBuffer.Get(), 0, nullptr, &boneCB, 0, 0);
	m_D3DDevice.GetDeviceContext()->VSSetConstantBuffers(1, 1, m_pBoneBuffer.GetAddressOf()); // b1 레지스터
	
	// boxHuman.UpdateBoneBuffer(m_D3DDevice.GetDeviceContext(), m_pBoneBuffer.Get());


	// Mesh 렌더링
	auto RenderMesh = [&](SkeletalMesh& mesh, const Matrix& world)
	{
		ConstantBuffer cb;
		cb.mWorld = XMMatrixTranspose(world); // 각 오브젝트 위치 
		cb.mView = XMMatrixTranspose(m_View);
		cb.mProjection = XMMatrixTranspose(m_Projection);
		cb.vLightDir = m_LightDir;
		cb.vLightColor = m_LightColor;
		cb.vEyePos = XMFLOAT4(m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z, 1.0f);
		cb.vAmbient = m_MaterialAmbient;
		cb.vDiffuse = m_LightDiffuse;
		cb.vSpecular = m_MaterialSpecular;
		cb.fShininess = m_Shininess;
		cb.gIsRigid = 1.0f;           // Rigid 모델이면 1 
		// cb.gRefBoneIndex = mesh.RefBoneIndex;      // 리지드일때 참조 본 인덱스 : 단일 본이면 0? 

		// SkeletalMesh 내부 Render에서 SubMesh 단위 렌더링과 Material 바인딩 처리
		mesh.Render(m_D3DDevice.GetDeviceContext(), cb, m_pConstantBuffer.Get(), m_pBoneBuffer.Get(), m_pSamplerLinear.Get());
	};
	
	RenderMesh(boxHuman, m_WorldChar);
	// PrintMatrix(m_WorldChar);

	// UI 그리기 
	Render_ImGui();


	// 스왑체인 교체 (화면 출력 : 백 버퍼 <-> 프론트 버퍼 교체)
	m_D3DDevice.EndFrame(); 
}

void TestApp::PrintMatrix(const Matrix& mat)
{
	char buf[512];
	sprintf_s(buf,
		"Matrix:\n"
		"[%f %f %f %f]\n"
		"[%f %f %f %f]\n"
		"[%f %f %f %f]\n"
		"[%f %f %f %f]\n",
		mat._11, mat._12, mat._13, mat._14,
		mat._21, mat._22, mat._23, mat._24,
		mat._31, mat._32, mat._33, mat._34,
		mat._41, mat._42, mat._43, mat._44);

	OutputDebugStringA(buf);
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
	ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_Always); // ImGuiCond_FirstUseEver

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
	HR_T(CompileShaderFromFile(L"../Shaders/10.VertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
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
	HR_T(CompileShaderFromFile(L"../Shaders/10.PixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));


	// ---------------------------------------------------------------
	// 상수 버퍼(Constant Buffer) 생성
	// ---------------------------------------------------------------
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer); 
	OutputDebugString((L"[sizeof(ConstantBuffer)] " + std::to_wstring(sizeof(ConstantBuffer)) + L"\n").c_str());
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&bd, nullptr, m_pConstantBuffer.GetAddressOf()));

	// 본 행렬용 상수 버퍼
	D3D11_BUFFER_DESC bdBone = {};
	bdBone.Usage = D3D11_USAGE_DEFAULT;
	bdBone.ByteWidth = sizeof(BoneMatrixContainer); // 128 x 4x4 Matrix
	OutputDebugString((L"[sizeof(BoneMatrixContainer)] " + std::to_wstring(sizeof(BoneMatrixContainer)) + L"\n").c_str());
	bdBone.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bdBone.CPUAccessFlags = 0;
	//bdBone.MiscFlags = 0;
	//bdBone.StructureByteStride = 0;

	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&bdBone, nullptr, m_pBoneBuffer.GetAddressOf()));
	


	// ---------------------------------------------------------------
	// 리소스 로드 
	// ---------------------------------------------------------------
	boxHuman.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/BoxHuman.fbx");



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
	m_WorldChar = XMMatrixTranslation(m_CharPos[0], m_CharPos[1], m_CharPos[2]);

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
	boxHuman.Clear();

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