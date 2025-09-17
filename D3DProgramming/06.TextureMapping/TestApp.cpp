#include "TestApp.h"
#include "../Common/Helper.h"

#include <string> 
#include <dxgi1_3.h>
#include <d3dcompiler.h>

#include <windows.h>

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "Psapi.lib")


// [ ���� ���� ]
struct Vertex
{
	Vector3 Pos;		// ��ġ ����
	Vector3 Normal;		// ���� ���� (���� ����)
};

// [ ��� ���� CB ] 
struct ConstantBuffer
{
	Matrix mWorld;			// ���� ��ȯ ��� : 64bytes
	Matrix mView;			// �� ��ȯ ���   : 64bytes
	Matrix mProjection;		// ���� ��ȯ ��� : 64bytes

	Vector4 vLightDir;		// ���� ����
	Vector4 vLightColor;	// ���� ����
	Vector4 vOutputColor;	// ��� ���� (��ü ���� ��)
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
	CheckDXGIDebug();	// DirectX ���ҽ� ���� üũ
}

void TestApp::Update()
{
	__super::Update();

	float totalTime = TimeSystem::m_Instance->TotalTime();

	// [ Cube ���� ��� ] -  Y�� �������� ȸ�� (Yaw, Pitch ����)
	XMMATRIX mRotYaw = XMMatrixRotationY(m_CubeYaw);
	XMMATRIX mRotPitch = XMMatrixRotationX(m_CubePitch);
	m_World = mRotPitch * mRotYaw;


	// [ ���� ó�� ]
	m_LightDirEvaluated = m_InitialLightDir; 


	// [ ī�޶� ���� ] 
	m_Camera.m_Position = { m_CameraPos[0], m_CameraPos[1], m_CameraPos[2] }; // ī�޶� ��ġ ����ȭ
	m_Camera.Update(m_Timer.DeltaTime());	// ī�޶� ���� ������Ʈ 
	m_Camera.GetViewMatrix(m_View);			// View ��� ����


	// [ ���� ��� ���� ] : ī�޶� Projection ��� ���� (FOV, Near, Far �ݿ�)
	float aspect = float(m_ClientWidth) / float(m_ClientHeight);
	m_Projection = XMMatrixPerspectiveFovLH(
		XMConvertToRadians(m_CameraFOV), // degree �� radian ��ȯ
		aspect,
		m_CameraNear,
		m_CameraFar
	);
}



// �� [ ������ ] 
void TestApp::Render()
{
	// 0. �׸� ��� ���� (���� Ÿ�� & �������ٽ� ����)
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

	// 1. ȭ�� �ʱ�ȭ (�÷� + ���� ����)
	const float clear_color_with_alpha[4] = { m_ClearColor.x , m_ClearColor.y , m_ClearColor.z, m_ClearColor.w };
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), clear_color_with_alpha);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // �������� 1.0f�� �ʱ�ȭ.


	// 2. ������ ���������� �������� ���� ( Draw ȣ�� ���� �ؾ��� )	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // ������ �̾ �׸� ��� (�ﰢ�� ����Ʈ ���)
	m_pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &m_VertextBufferStride, &m_VertextBufferOffset); // (Stride: ���� ũ��, Offset: ���� ��ġ)
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());
	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_pDeviceContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());
	m_pDeviceContext->PSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());


	// 3. ��� ���� ������Ʈ & �׸��� 
	ConstantBuffer cb;
	cb.mWorld = XMMatrixTranspose(m_World);
	cb.mView = XMMatrixTranspose(m_View);
	cb.mProjection = XMMatrixTranspose(m_Projection);
	cb.vLightDir = m_LightDirEvaluated; // ���� ����Ʈ 
	cb.vLightColor = m_LightColor;		
	cb.vOutputColor = XMFLOAT4(0, 0, 0, 0);
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);


	// 4. ����Ʈ ǥ�� (���� ��ġ�� ���� ť��� ������)
	XMMATRIX mLight = XMMatrixTranslationFromVector(5.0f * XMLoadFloat4(&m_LightDirEvaluated));
	XMMATRIX mLightScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	mLight = mLightScale * mLight;

	// ��� ���� ���� (���� ��ġ + ���� �ݿ�)
	cb.mWorld = XMMatrixTranspose(mLight);
	cb.vOutputColor = m_LightColor;
	m_pDeviceContext->UpdateSubresource(m_pConstantBuffer.Get(), 0, nullptr, &cb, 0, 0);

	// ����Ʈ ���� �ȼ� ���̴� ���� �� ť�� ������
	m_pDeviceContext->PSSetShader(m_pPixelShaderSolid.Get(), nullptr, 0);
	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);

	// 4. UI �׸��� 
	Render_ImGui();


	// 5. ����ü�� ��ü (ȭ�� ��� : �� ���� <-> ����Ʈ ���� ��ü)
	m_pSwapChain->Present(0, 0);
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

	// Cube ȸ��
	ImGui::Text("Cube Rotation");
	ImGui::SliderAngle("Yaw (Y axis)", &m_CubeYaw, -180.0f, 180.0f);
	ImGui::SliderAngle("Pitch (X axis)", &m_CubePitch, -180.0f, 180.0f);

	ImGui::Text("");

	// [ Light UI ]
	ImGui::Text("[ Light ]");

	// Light ����
	ImGui::Text("Light Color");
	ImGui::ColorEdit3("Light Color", (float*)&m_LightColor);

	// Light ����
	ImGui::Text("Light Direction");
	ImGui::SliderFloat3("Light Dir", (float*)&m_InitialLightDir, -1.0f, 1.0f);

	ImGui::Text("");

	// [ Camera UI ]
	ImGui::Text("[ Camera ]");
	 
	// ī�޶� ���� ��ġ (x,y,z)
	ImGui::Text("1. Camera Pos");
	ImGui::SliderFloat3("Camera Pos", m_CameraPos, -100.0f, 100.0f);

	// ī�޶� FOV (degree)
	ImGui::Text("2. FOV (deg)");
	ImGui::SliderFloat("FOV (deg)", &m_CameraFOV, 10.0f, 120.0f);

	// ī�޶� Near, Far
	ImGui::Text("3. Near");
	ImGui::SliderFloat("Near", &m_CameraNear, 0.01f, 10.0f);

	ImGui::Text("4. Far");
	ImGui::SliderFloat("Far", &m_CameraFar, 10.0f, 5000.0f);


	// [ �� ] 
	ImGui::End();

	// [ ImGui ������ ���� ������ ]
	ImGui::Render();  // ImGui ���ο��� ������ �����͸� �غ�
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData()); // DX11�� ���� ȭ�鿡 ���
}


// �� [ Direct3D11 �ʱ�ȭ ] 
bool TestApp::InitD3D()
{
	
	HRESULT hr = 0; // ����� : DirectX �Լ��� HRESULT ��ȯ


	// =====================================
	// 0. DXGI Factory ���� �� ù ��° Adapter �������� (ImGui)
	// =====================================
	HR_T(CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)m_pDXGIFactory.GetAddressOf()));
	HR_T(m_pDXGIFactory->EnumAdapters(0, reinterpret_cast<IDXGIAdapter**>(m_pDXGIAdapter.GetAddressOf()))); 



	// =====================================
	// 1. ����̽� / ����̽� ���ؽ�Ʈ ����
	// =====================================

	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG; // ����� ���̾� Ȱ��ȭ (���� �޽��� ���)
#endif
	// �׷��� ī�� �ϵ������ �������� ȣȯ�Ǵ� ���� ���� DirectX ��ɷ����� �����Ͽ� ����̹��� �۵� �Ѵ�.
	// �������̽��� Direc3D11 ������ GPU����̹��� D3D12 ����̹��� �۵��Ҽ��� �ִ�.
	D3D_FEATURE_LEVEL featureLevels[] = // index 0���� ������� �õ��Ѵ�.
	{ 
			D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0, D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0
	};
	D3D_FEATURE_LEVEL actualFeatureLevel; // ���� ��ó ������ ������ ����

	HR_T(D3D11CreateDevice(
		nullptr,						// �⺻ ����� ���
		D3D_DRIVER_TYPE_HARDWARE,		// �ϵ���� ������ ���
		0,								// ����Ʈ���� ����̹� ����
		creationFlags,					// ����� �÷���
		featureLevels,					// Feature Level ���� 
		ARRAYSIZE(featureLevels),
		D3D11_SDK_VERSION,				// SDK ����
		m_pDevice.GetAddressOf(),		// ����̽� ��ȯ
		&actualFeatureLevel,			// Feature Level ��ȯ
		m_pDeviceContext.GetAddressOf() // ����̽� ���ý�Ʈ ��ȯ 
	));




	// =====================================
	// 2. DXGI Factory ���� �� ����ü�� �غ�
	// =====================================

	UINT dxgiFactoryFlags = 0;
#ifdef _DEBUG
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ComPtr<IDXGIFactory2> pFactory;
	HR_T(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(pFactory.GetAddressOf())));

	// ����ü��(�����) ���� 
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = 2;									// ����� ���� 
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.Width = m_ClientWidth;
	swapChainDesc.Height = m_ClientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				// ����� ����: 32��Ʈ RGBA
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;	// ������� �뵵: ����Ÿ�� ���
	swapChainDesc.SampleDesc.Count = 1;								// ��Ƽ���ø�(��Ƽ���ϸ����) ��� ���� (�⺻: 1, �� ��)
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;				// Recommended for flip models
	swapChainDesc.Stereo = FALSE;									// 3D ��Ȱ��ȭ
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;	// ��ü ȭ�� ��ȯ ���
	swapChainDesc.Scaling = DXGI_SCALING_NONE;						// â�� ũ��� �� ������ ũ�Ⱑ �ٸ� ��. �ڵ� �����ϸ�X 

	HR_T(pFactory->CreateSwapChainForHwnd(
		m_pDevice.Get(),
		m_hWnd,
		&swapChainDesc,
		nullptr,
		nullptr,
		m_pSwapChain.GetAddressOf()
	));




	// =====================================
    // 3. ����Ÿ�ٺ�(Render Target View) ����
    // =====================================
	
	ComPtr<ID3D11Texture2D> pBackBufferTexture; 
	HR_T(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)(pBackBufferTexture.GetAddressOf())));		// ����ü���� 0�� ����(�����) ������
	HR_T(m_pDevice->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, m_pRenderTargetView.GetAddressOf())); // ����� �ؽ�ó�� ������� ����Ÿ�ٺ� ����
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), NULL); // ?



	// =====================================
	// 4. ����Ʈ(Viewport) ����
	// =====================================

	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;                        // ����Ʈ ���� X
	viewport.TopLeftY = 0;                        // ����Ʈ ���� Y
	viewport.Width = (float)m_ClientWidth;        // ����Ʈ �ʺ�
	viewport.Height = (float)m_ClientHeight;      // ����Ʈ ����
	viewport.MinDepth = 0.0f;                     // ���� ���� �ּҰ�
	viewport.MaxDepth = 1.0f;                     // ���� ���� �ִ밪

	// �����Ͷ�����(RS: Rasterizer) �ܰ迡 ����Ʈ ����
	m_pDeviceContext->RSSetViewports(1, &viewport);




	// =====================================
	// 5. ����&���ٽ� ���� �� �� ����
	// =====================================

	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = m_ClientWidth;
	descDepth.Height = m_ClientHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24��Ʈ ���� + 8��Ʈ ���ٽ�
	descDepth.SampleDesc.Count = 1;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	ComPtr<ID3D11Texture2D> pTextureDepthStencil;
	HR_T(m_pDevice->CreateTexture2D(&descDepth, nullptr, pTextureDepthStencil.GetAddressOf()));

	// ���� ���ٽ� �� 
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



// �� [ Scene �ʱ�ȭ ] 
bool TestApp::InitScene()
{

	HRESULT hr = 0;                       // DirectX �Լ� �����
	ID3D10Blob* errorMessage = nullptr;   // ���̴� ������ ���� �޽��� ����	


	// ================================================================
	// 1. ����(Vertex) ������ �غ� �� ���� ���� ����
	// ================================================================
	
	// ť�� ���� �� �鿡 ���� ���� ������ (��ġ + ��ֺ���)
	// Local Space(�� ��ǥ��) ����
	Vertex vertices[] =
	{
		// ���� (Normal Y+)
		{ Vector3(-1.0f, 1.0f, -1.0f),	Vector3(0.0f, 1.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),	Vector3(0.0f, 1.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),	Vector3(0.0f, 1.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),	Vector3(0.0f, 1.0f, 0.0f) },

		// �Ʒ��� (Normal Y-)
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, -1.0f, 0.0f) },	
		{ Vector3(1.0f, -1.0f, -1.0f),	Vector3(0.0f, -1.0f, 0.0f) },
		{ Vector3(1.0f, -1.0f, 1.0f),	Vector3(0.0f, -1.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, 1.0f),	Vector3(0.0f, -1.0f, 0.0f) },

		// ���ʸ� (Normal X-)
		{ Vector3(-1.0f, -1.0f, 1.0f),	Vector3(-1.0f, 0.0f, 0.0f) },
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(-1.0f, 0.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, -1.0f),	Vector3(-1.0f, 0.0f, 0.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),	Vector3(-1.0f, 0.0f, 0.0f) },

		// �����ʸ� (Normal X+)
		{ Vector3(1.0f, -1.0f, 1.0f),	Vector3(1.0f, 0.0f, 0.0f) },
		{ Vector3(1.0f, -1.0f, -1.0f),	Vector3(1.0f, 0.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),	Vector3(1.0f, 0.0f, 0.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),	Vector3(1.0f, 0.0f, 0.0f) },

		// �޸� (Normal Z-)
		{ Vector3(-1.0f, -1.0f, -1.0f), Vector3(0.0f, 0.0f, -1.0f) }, 
		{ Vector3(1.0f, -1.0f, -1.0f),	Vector3(0.0f, 0.0f, -1.0f) },
		{ Vector3(1.0f, 1.0f, -1.0f),	Vector3(0.0f, 0.0f, -1.0f) },
		{ Vector3(-1.0f, 1.0f, -1.0f),	Vector3(0.0f, 0.0f, -1.0f) },

		// �ո� (Normal Z+)
		{ Vector3(-1.0f, -1.0f, 1.0f),	Vector3(0.0f, 0.0f, 1.0f) },
		{ Vector3(1.0f, -1.0f, 1.0f),	Vector3(0.0f, 0.0f, 1.0f) },
		{ Vector3(1.0f, 1.0f, 1.0f),	Vector3(0.0f, 0.0f, 1.0f) },
		{ Vector3(-1.0f, 1.0f, 1.0f),	Vector3(0.0f, 0.0f, 1.0f) },
	};
	m_VertexCount = ARRAYSIZE(vertices); // ���� ���� ����


	// ���� ���� �Ӽ� ���� 
	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = sizeof(Vertex) * m_VertexCount;    // ���� ũ�� (���� ũ�� �� ���� ����)
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;          // �뵵: ���� ����
	bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU�� �а� ���� �⺻ ����
	bd.CPUAccessFlags = 0;

	// �ʱ� ������ ���޿� ����ü
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices; // ���� �迭 ������ �Ҵ� 

	// ���� ���� ����
	HR_T(m_pDevice->CreateBuffer(&bd, &vbData, m_pVertexBuffer.GetAddressOf()));

	// ���ؽ� ���� ����
	m_VertextBufferStride = sizeof(Vertex); // ���� �ϳ��� ũ��
	m_VertextBufferOffset = 0;              // ���� ������ (0)



	// ================================================================
	// 2. �ε��� ���� ����
	// ================================================================

	WORD indices[] =
	{
		// ����, �Ʒ���, ����, ������, ��, ��
		3,1,0, 2,1,3,
		6,4,5, 7,4,6,
		11,9,8, 10,9,11,
		14,12,13, 15,12,14,
		19,17,16, 18,17,19,
		22,20,21, 23,20,22
	};
	m_nIndices = ARRAYSIZE(indices);	// �ε��� ���� ����

	// �ε��� ���� �Ӽ� ����
	bd = {};
	bd.ByteWidth = sizeof(WORD) * m_nIndices;		  // ��ü �ε��� �迭 ũ��
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;           // �ε��� ���۷� ���
	bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU���� �б� ����(����X)
	bd.CPUAccessFlags = 0;

	// �ʱ� ������ ����
	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	// �ε��� ���� ����
	HR_T(m_pDevice->CreateBuffer(&bd, &ibData, m_pIndexBuffer.GetAddressOf()));



	// ================================================================
	// 3. ���ؽ� ���̴�(Vertex Shader) ������ �� ����
	// ================================================================

	ComPtr<ID3DBlob> vertexShaderBuffer; // �����ϵ� ���ؽ� ���̴� �ڵ�(hlsl) ���� ����
	
	// ' HLSL ���Ͽ��� main �Լ��� vs_4_0 �԰����� ������ '
	HR_T(CompileShaderFromFile(L"../Shaders/06.BasicVertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, m_pVertexShader.GetAddressOf()));



	// ================================================================
	// 4. �Է� ���̾ƿ�(Input Layout) ����
	// ================================================================

	// ���� ���̴��� �Է����� ���� ������ ���� ����
	// { SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate }

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // POSITION : float3 (12����Ʈ)
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },  // NORMAL   : float3 (12����Ʈ, ������ 12)
	};

	// ���ؽ� ���̴��� Input�ñ״�ó�� ���� ��ȿ�� �˻� �� -> InputLayout ����
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));



	// ================================================================
	// 5. �ȼ� ���̴�(Pixel Shader) ������ �� ����
	// ================================================================

	ComPtr<ID3DBlob> pixelShaderBuffer; // �����ϵ� ���ؽ� �ȼ� �ڵ�(hlsl) ���� ����

	// ' HLSL ���Ͽ��� main �Լ��� ps_4_0 �԰����� ������ '
	HR_T(CompileShaderFromFile(L"../Shaders/06.BasicPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShader.GetAddressOf()));

	// ��(����Ʈ) ��ġ�� �ð�ȭ�� ���� �ȼ� ���̴�
	HR_T(CompileShaderFromFile(L"../Shaders/06.SolidPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(),pixelShaderBuffer->GetBufferSize(), NULL, m_pPixelShaderSolid.GetAddressOf()));



	// ================================================================
	// 6.  ��� ����(Constant Buffer) ����
	// ================================================================

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer); // World, View, Projection + Light
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bd, nullptr, m_pConstantBuffer.GetAddressOf()));



	// ================================================================
	// 7. ���(World, View, Projection) ����
	// ================================================================
	
	m_World = XMMatrixIdentity(); // ���� ��� 

	// ī�޶�(View)
	XMVECTOR Eye = XMVectorSet(0.0f, 4.0f, -10.0f, 0.0f);	// ī�޶� ��ġ
	XMVECTOR At = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// ī�޶� �ٶ󺸴� ��ġ
	XMVECTOR Up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);		// ī�޶��� ���� ���� 
	m_View = XMMatrixLookAtLH(Eye, At, Up);					// �޼� ��ǥ��(LH) ���� 

	// �������(Projection)
	m_Projection = XMMatrixPerspectiveFovLH(
		XM_PIDIV2,                             // FOV( 90��: ���� �þ߰� ) 
		m_ClientWidth / (FLOAT)m_ClientHeight, // ȭ�� ��Ⱦ��
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
	// 1. ImGui ���� Ȯ�� 
	IMGUI_CHECKVERSION();   

	// 2. ImGui ���ؽ�Ʈ ����
	ImGui::CreateContext(); 

	// 3. ImGui ��Ÿ�� ����
	ImGui::StyleColorsDark(); // ImGui::StyleColorsLight();

	// 4. �÷��� �� ������ �鿣�� �ʱ�ȭ
	ImGui_ImplWin32_Init(m_hWnd);												// Win32 �÷����� �ʱ�ȭ (������ �ڵ� �ʿ�)
	ImGui_ImplDX11_Init(this->m_pDevice.Get(), this->m_pDeviceContext.Get());	// DirectX11 �������� �ʱ�ȭ


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