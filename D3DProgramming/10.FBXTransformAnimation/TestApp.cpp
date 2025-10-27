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
	UninitScene();      // ���ҽ� ���� 
	CheckDXGIDebug();	// DirectX ���ҽ� ���� üũ
}

void TestApp::Update()
{
	__super::Update();

	float totalTime = TimeSystem::m_Instance->TotalTime();

	m_Camera.GetViewMatrix(m_View);			// View ��� ����
}     


// �� [ ������ ] 
void TestApp::Render()
{
	// ȭ�� �ʱ�ȭ
	const float clearColor[4] = { m_ClearColor.x, m_ClearColor.y, m_ClearColor.z, m_ClearColor.w };
	m_D3DDevice.BeginFrame(clearColor);

	m_D3DDevice.GetDeviceContext()->IASetInputLayout(m_pInputLayout.Get());
	m_D3DDevice.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_D3DDevice.GetDeviceContext()->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	m_D3DDevice.GetDeviceContext()->PSSetSamplers(0, 1, m_pSamplerLinear.GetAddressOf());

	// Mesh ������
	auto RenderMesh = [&](StaticMesh& mesh, const Matrix& world)
	{
		ConstantBuffer cb;
		cb.mWorld = XMMatrixTranspose(world); // �� ������Ʈ ��ġ 
		cb.mView = XMMatrixTranspose(m_View);
		cb.mProjection = XMMatrixTranspose(m_Projection);
		cb.vLightDir = m_LightDir;
		cb.vLightColor = m_LightColor;
		cb.vEyePos = XMFLOAT4(m_Camera.m_Position.x, m_Camera.m_Position.y, m_Camera.m_Position.z, 1.0f);
		cb.vAmbient = m_MaterialAmbient;
		cb.vDiffuse = m_LightDiffuse;
		cb.vSpecular = m_MaterialSpecular;
		cb.fShininess = m_Shininess;

		for (StaticMeshSection& sub : mesh.m_SubMeshes)
		{
			// Material �ؽ�ó ���ε�
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

			// SubMesh ������
			sub.Render(m_D3DDevice.GetDeviceContext(), mesh.m_Materials[sub.m_MaterialIndex], cb, m_pConstantBuffer.Get(), m_pSamplerLinear.Get());
		}
	};

	RenderMesh(treeMesh, m_WorldTree);
	RenderMesh(charMesh, m_WorldChar);
	RenderMesh(zeldaMesh, m_WorldZelda);


	// UI �׸��� 
	Render_ImGui();


	// ����ü�� ��ü (ȭ�� ��� : �� ���� <-> ����Ʈ ���� ��ü)
	m_D3DDevice.EndFrame(); 
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
	ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_Always); // ImGuiCond_FirstUseEver

	// [ Control UI ]
	ImGui::Begin("Controllor");


	// -----------------------------
	// [ Light ]
	// -----------------------------
	ImGui::Text(" ");
	ImGui::Text("[ Light ]");

	// ���� ����
	ImGui::ColorEdit3("Light Color", (float*)&m_LightColor);

	// ���� ���� 
	ImGui::DragFloat3("Light Dir", (float*)&m_LightDir, 0.01f, -1.0f, 1.0f);

	// �ֺ��� / ���ݻ� / ���ݻ� ���
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

	// ī�޶� ��ġ
	ImGui::Text("Position (XYZ)");
	if (ImGui::DragFloat3("Position", &m_Camera.m_Position.x, 0.5f))
	{
		// ��ġ ���� �� ��� World ��� ������Ʈ
		m_Camera.m_World = Matrix::CreateFromYawPitchRoll(m_Camera.m_Rotation) * Matrix::CreateTranslation(m_Camera.m_Position);
	}

	// ī�޶� ȸ�� (�� ����)
	Vector3 rotationDegree = 
	{
		XMConvertToDegrees(m_Camera.m_Rotation.x),
		XMConvertToDegrees(m_Camera.m_Rotation.y),
		XMConvertToDegrees(m_Camera.m_Rotation.z)
	};

	ImGui::Text("Rotation (Degrees)");
	if (ImGui::DragFloat3("Rotation", &rotationDegree.x, 0.5f))
	{
		// �Է°��� �������� ��ȯ�Ͽ� ī�޶� ����
		m_Camera.m_Rotation.x = XMConvertToRadians(rotationDegree.x);
		m_Camera.m_Rotation.y = XMConvertToRadians(rotationDegree.y);
		m_Camera.m_Rotation.z = XMConvertToRadians(rotationDegree.z);

		// ȸ�� ���� �� World ��� ����
		m_Camera.m_World = Matrix::CreateFromYawPitchRoll(m_Camera.m_Rotation) * Matrix::CreateTranslation(m_Camera.m_Position);
	}

	// �̵� �ӵ� ����
	ImGui::Text("Move Speed");
	ImGui::SliderFloat("Speed", &m_Camera.m_MoveSpeed, 10.0f, 1000.0f, "%.1f");

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

	m_WorldTree = XMMatrixTranslation(m_TreePos[0], m_TreePos[1], m_TreePos[2]);
	m_WorldChar = XMMatrixTranslation(m_CharPos[0], m_CharPos[1], m_CharPos[2]);
	m_WorldZelda = XMMatrixTranslation(m_ZeldaPos[0], m_ZeldaPos[1], m_ZeldaPos[2]);

	// ī�޶�(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// ī�޶� ��ġ
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// ī�޶� �ٶ󺸴� ��ġ
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// ī�޶��� ���� ���� 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// �޼� ��ǥ��(LH) ���� 

	// �������(Projection) : ī�޶��� �þ߰�(FOV), ȭ�� ��Ⱦ��, Near, Far 
	m_Projection = XMMatrixPerspectiveFovLH(XM_PIDIV4,m_ClientWidth / (FLOAT)m_ClientHeight, m_CameraNear, m_CameraFar);


	return true;
}

void TestApp::UninitScene()
{
	// ���÷�, �������, ���̴�, �Է·��̾ƿ� ����
	m_pSamplerLinear.Reset();
	m_pConstantBuffer.Reset();
	m_pVertexShader.Reset();
	m_pPixelShader.Reset();
	m_pInputLayout.Reset();

	// Mesh, Material ����
	treeMesh.Clear();
	charMesh.Clear();
	zeldaMesh.Clear();

	// �⺻ �ؽ�ó ����
	Material::DestroyDefaultTextures();
}



// [ ImGui ]
bool TestApp::InitImGUI()
{
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