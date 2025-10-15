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


// [ ��� ���� CB ] 
struct ConstantBuffer
{
	Matrix mWorld;			// ���� ��ȯ ��� : 64bytes
	Matrix mView;			// �� ��ȯ ���   : 64bytes
	Matrix mProjection;		// ���� ��ȯ ��� : 64bytes

	Vector4 vLightDir;		// ���� ����
	Vector4 vLightColor;	// ���� ����
	Vector4 vOutputColor;	// ��� ���� 

	Vector4 vEyePos;      // ī�޶� ��ġ

	Vector4 vAmbient;     // ��Ƽ���� Ambient
	Vector4 vDiffuse;     // ��Ƽ���� Diffuse
	Vector4 vSpecular;    // ��Ƽ���� Specular
	float   fShininess = 40.0f;   // ��¦�� ����
	float   pad[3];       // 16����Ʈ ���� �е�
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
	CheckDXGIDebug();	// DirectX ���ҽ� ���� üũ
	m_D3DDevice.Cleanup();
}

void TestApp::Update()
{
	__super::Update();

	float totalTime = TimeSystem::m_Instance->TotalTime();

	// [ ���� ó�� ]
	m_LightDirEvaluated = m_InitialLightDir; 

	// [ ī�޶� ���� ] 
	m_Camera.m_Position = { m_CameraPos[0], m_CameraPos[1], m_CameraPos[2] }; 
	m_Camera.m_Rotation.y = m_CameraYaw; 
	m_Camera.m_Rotation.x = m_CameraPitch; 

	m_Camera.Update(m_Timer.DeltaTime());	// ī�޶� ���� ������Ʈ 
	m_Camera.GetViewMatrix(m_View);			// View ��� ����


	// [ ���� ��� ���� ] : ī�޶� Projection ��� ���� (FOV, Near, Far �ݿ�)
	m_Projection = XMMatrixPerspectiveFovLH
	(
		XMConvertToRadians(m_CameraFOV), // degree �� radian ��ȯ
		float(m_ClientWidth) / float(m_ClientHeight),
		m_CameraNear,
		m_CameraFar
	);
}     



// �� [ ������ ] 
void TestApp::Render()
{
	// 0. �׸� ��� ���� (���� Ÿ�� & �������ٽ� ����) �� ȭ�� �ʱ�ȭ (�÷� + ���� ����)
	const float clearColor[4] = { m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w };
	m_D3DDevice.BeginFrame(clearColor);


	// 1. ������ ���������� �������� ���� ( Draw ȣ�� ���� �����ϰ� ȣ�� �ؾ��� )	

	// CB ������Ʈ
	ConstantBuffer cb;
	cb.mWorld		= XMMatrixTranspose(m_World);
	cb.mView		= XMMatrixTranspose(m_View);
	cb.mProjection	= XMMatrixTranspose(m_Projection);
	cb.vLightDir	= m_LightDirEvaluated;
	cb.vLightColor	= m_LightColor;
	cb.vEyePos		= XMFLOAT4(m_CameraPos[0], m_CameraPos[1], m_CameraPos[2], 1.0f);

	// ��Ƽ���� + ���� �Ķ����
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

	//m_pDeviceContext->PSSetShaderResources(0, 1, m_pTexture.GetAddressOf());   // ť�� �ؽ�ó
	//m_pDeviceContext->PSSetShaderResources(1, 1, m_pNormal.GetAddressOf());    // ��ָ�
	//m_pDeviceContext->PSSetShaderResources(2, 1, m_pSpecular.GetAddressOf());  // ����ŧ����

	m_D3DDevice.GetDeviceContext()->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	// ConstantBuffer �� ���� �� draw
	treeMesh.Render(m_D3DDevice.GetDeviceContext());
	charMesh.Render(m_D3DDevice.GetDeviceContext());
	zeldaMesh.Render(m_D3DDevice.GetDeviceContext());

	// m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0); // draw


	// 2. UI �׸��� 
	Render_ImGui();


	// 3. ����ü�� ��ü (ȭ�� ��� : �� ���� <-> ����Ʈ ���� ��ü)
	m_D3DDevice.EndFrame(); // m_pSwapChain->Present(0, 0);
}



// �� [ ImGui ] - UI ������ �غ� �� ������
void TestApp::Render_ImGui()
{
	// ImGui�� �Է�/��� ����ü�� �����´�.  (void)io; -> ������� ���� �� ��� ���ſ�
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Ű����� UI ���� 
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // �����е�� UI ���� 


	// [ ���ο� ImGui ������ ���� ]
	ImGui_ImplDX11_NewFrame();		// DirectX11 �������� ������ �غ�
	ImGui_ImplWin32_NewFrame();		// Win32 �÷����� ������ �غ�
	ImGui::NewFrame();				// ImGui ���� ������ �ʱ�ȭ


	// �ʱ� â ũ�� ����
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
	ImGui::ColorEdit3("Ambient (k_a)", (float*)&m_MaterialAmbient);   // ȯ�汤 �ݻ� ��� 
																	  // ���ݻ� ��� -> Diffuse �ؽ�ó 
	ImGui::ColorEdit3("Specular (k_s)", (float*)&m_MaterialSpecular); // ���ݻ� ��� 
	
	ImGui::Text("");

	// [ Camera ]
	ImGui::Text("[ Camera ]");
	ImGui::SliderFloat3("Camera Pos", m_CameraPos, -1000.0f, 1000.0f);

	ImGui::SliderAngle("Camera Yaw (Y axis)", &m_CameraYaw, -180.0f, 180.0f);
	ImGui::SliderAngle("CameraPitch (X axis)", &m_CameraPitch, -90.0f, 90.0f);

	// [ �� ] 
	ImGui::End();

	// [ ImGui ������ ���� ������ ]
	ImGui::Render();  // ImGui ���ο��� ������ �����͸� �غ�
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // DX11�� ���� ȭ�鿡 ���
}



// �� [ Scene �ʱ�ȭ ] 
bool TestApp::InitScene()
{
	HRESULT hr = 0;                       // DirectX �Լ� �����
	ID3D10Blob* errorMessage = nullptr;   // ���̴� ������ ���� �޽��� ����	


	// ---------------------------------------------------------------
	// ���ؽ� ���̴�(Vertex Shader) ������ �� ����
	// ---------------------------------------------------------------
	ComPtr<ID3DBlob> vertexShaderBuffer; 
	HR_T(CompileShaderFromFile(L"../Shaders/09.VertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));


	// ---------------------------------------------------------------
	// �Է� ���̾ƿ�(Input Layout) ����
	//---------------------------------------------------------------
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // POSITION : float3 (12����Ʈ)
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },  // NORMAL   : float3 (12����Ʈ, ������ 12)
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // TEXCOORD : float2 (8����Ʈ, ������ 24)
		{ "TANGENT",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 }, 
		{ "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	HR_T(m_D3DDevice.GetDevice()->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));


	// ---------------------------------------------------------------
	// �ȼ� ���̴�(Pixel Shader) ������ �� ����
	// ---------------------------------------------------------------
	ComPtr<ID3DBlob> pixelShaderBuffer; 
	HR_T(CompileShaderFromFile(L"../Shaders/09.PixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_D3DDevice.GetDevice()->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));


	// ---------------------------------------------------------------
	// ��� ����(Constant Buffer) ����
	// ---------------------------------------------------------------
	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer); // World, View, Projection + Light
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HR_T(m_D3DDevice.GetDevice()->CreateBuffer(&bd, nullptr, m_pConstantBuffer.GetAddressOf()));

	
	// ---------------------------------------------------------------
	// ���ҽ� �ε� 
	// ---------------------------------------------------------------
	treeMesh.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/Tree.fbx");
	charMesh.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/Character.fbx");
	zeldaMesh.LoadFromFBX(m_D3DDevice.GetDevice(), "../Resource/zeldaPosed001.fbx");


	// ---------------------------------------------------------------
	// ���÷� ����
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
	// ���(World, View, Projection) ����
	// ---------------------------------------------------------------
	m_World = XMMatrixIdentity(); // ���� ��� 

	// ī�޶�(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// ī�޶� ��ġ
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// ī�޶� �ٶ󺸴� ��ġ
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// ī�޶��� ���� ���� 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// �޼� ��ǥ��(LH) ���� 

	// �������(Projection) : ī�޶��� �þ߰�(FOV), ȭ�� ��Ⱦ��, Near, Far 
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4,m_ClientWidth / (FLOAT)m_ClientHeight, 0.01f, 100.0f);


	return true;
}


// [ ImGui ]
bool TestApp::InitImGUI()
{
	// ???? 
	// DXGI Factory ���� �� ù ��° Adapter �������� (ImGui)
	// HR_T(CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)m_pDXGIFactory.GetAddressOf()));
	// HR_T(m_pDXGIFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(m_pDXGIAdapter.GetAddressOf())));

	// 1. ImGui ���� Ȯ�� 
	IMGUI_CHECKVERSION();   

	// 2. ImGui ���ؽ�Ʈ ����
	ImGui::CreateContext(); 

	// 3. ImGui ��Ÿ�� ����
	ImGui::StyleColorsDark(); // ImGui::StyleColorsLight();

	// 4. �÷��� �� ������ �鿣�� �ʱ�ȭ
	ImGui_ImplWin32_Init(m_hWnd);												// Win32 �÷����� �ʱ�ȭ (������ �ڵ� �ʿ�)
	ImGui_ImplDX11_Init(this->m_D3DDevice.GetDevice(), this->m_D3DDevice.GetDeviceContext());	// DirectX11 �������� �ʱ�ȭ


	return true;
}

void TestApp::UninitImGUI()
{
	ImGui_ImplDX11_Shutdown();	// DX11�� ImGui ������ ����
	ImGui_ImplWin32_Shutdown(); // Win32 �÷����� ImGui ����
	ImGui::DestroyContext();	// ImGui ���ؽ�Ʈ ����
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK TestApp::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	return __super::WndProc(hWnd, message, wParam, lParam);
}