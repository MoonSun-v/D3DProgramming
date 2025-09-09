#include "TestApp.h"
#include "../Common/Helper.h"

#include <dxgi1_3.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib,"d3dcompiler.lib")



// [ ���� ���� ]
struct Vertex
{
	Vector3 position;	// ��ġ ����
	Vector4 color;		// ���� ����

	Vertex(float x, float y, float z) : position(x, y, z) {}
	Vertex(Vector3 position) : position(position) {}
	Vertex(Vector3 position, Vector4 color): position(position), color(color) {}
};

// [ ��� ���� CB ]
struct ConstantBuffer
{
	Matrix mWorld;       // ���� ��ȯ ���
	Matrix mView;        // �� ��ȯ ���
	Matrix mProjection;  // ���� ��ȯ ���
};

TestApp::TestApp(HINSTANCE hInstance) : GameApp(hInstance)
{

}

TestApp::~TestApp()
{
	UninitScene();
	UninitD3D();
}

bool TestApp::Initialize(UINT Width, UINT Height)
{
	__super::Initialize(Width, Height);

	if (!InitD3D())		return false;
	if (!InitScene())	return false;

	return true;
}

void TestApp::Uninitialize()
{
	CheckDXGIDebug();	// // DirectX ���ҽ� ���� üũ
}

void TestApp::Update()
{
	__super::Update();

	float totalTime = TimeSystem::m_Instance->TotalTime();


	// [ 1��° Cube ] : �ܼ��� Y�� ȸ�� 
	m_World1 = XMMatrixRotationY(totalTime);


	// [ 2��° Cube ] : �˵� ȸ�� + ���ڸ� ȸ�� + ������ ��� + �̵�
	// mOrbit : �ٸ� ���� �������� ȸ�� 
	XMMATRIX spin2 = XMMatrixRotationZ(-totalTime);					// ���ڸ� Z�� ȸ��
	XMMATRIX orbit2 = XMMatrixRotationY(-totalTime * 1.5f);			// �߽��� �������� Y�� �˵� ȸ��
	XMMATRIX translate2 = XMMatrixTranslation(-4.0f, 0.0f, 0.0f);	// �������� �̵�
	XMMATRIX scale2 = XMMatrixScaling(0.3f, 0.3f, 0.3f);			// ũ�� ���

	// scale -> spin -> translate -> orbit 
	XMMATRIX temp2_1 = XMMatrixMultiply(scale2, spin2);
	XMMATRIX temp2_2 = XMMatrixMultiply(XMMatrixMultiply(scale2, spin2), translate2);
	m_World2 = XMMatrixMultiply(temp2_2, orbit2);					// ���� ���� ���


	// [ 3��° Cube ] : 2�� ť���� �ڽ�
	XMMATRIX spin3 = XMMatrixRotationZ(totalTime);					// ���ڸ� Z�� ȸ��
	XMMATRIX orbit3 = XMMatrixRotationY(totalTime * 3.0f);			// 2�� ť�� �߽��� �������� �˵� ȸ��
	XMMATRIX translate3 = XMMatrixTranslation(-2.0f, 0.0f, 0.0f);	// �������� �̵�
	XMMATRIX scale3 = XMMatrixScaling(0.5f, 0.5f, 0.5f);			// ũ�� ���

	// scale -> spin -> translate -> orbit -> orbit 
	XMMATRIX temp3_1 = XMMatrixMultiply(scale3, spin3);
	XMMATRIX temp3_2 = XMMatrixMultiply(temp3_1, translate3);

	XMMATRIX local3 = XMMatrixMultiply(temp3_2, orbit3);

	m_World3 = XMMatrixMultiply(local3, m_World2);	// 2�� ť���� �ڽ����� ����
}



// �� [ ������ ] 
void TestApp::Render()
{
	// 0. �׸� ��� ����
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), m_pDepthStencilView.Get());

	// 1. ȭ�� ĥ�ϱ�
	float color[4] = { 0.80f, 0.92f, 1.0f, 1.0f }; //  Light Sky Blue 
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), color);
	m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0); // �������� 1.0f�� �ʱ�ȭ.


	// 2. ������ ���������� ����
	// ( Draw�迭 �Լ� ȣ�� �� -> ������ ���������ο� �ʼ� �������� ���� �ؾ��� )	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // ������ �̾ �׸� ��� (�ﰢ�� ����Ʈ ���)
	m_pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &m_VertextBufferStride, &m_VertextBufferOffset); // (Stride: ���� ũ��, Offset: ���� ��ġ)
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());
	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_pDeviceContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);
	m_pDeviceContext->VSSetConstantBuffers(0, 1, m_pConstantBuffer.GetAddressOf());



	// 3. �׸��� (�ε��� ���� ���)
	// �� �ε��� ���� ���� ������ ����� ��� -> Draw() ���

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



	// 4. ����ü�� ��ü (ȭ�� ��� : �� ���� <-> ����Ʈ ���� ��ü)
	m_pSwapChain->Present(0, 0);
}



// �� [ Direct3D11 �ʱ�ȭ ] 
bool TestApp::InitD3D()
{
	
	HRESULT hr = 0; // ����� : DirectX �Լ��� HRESULT ��ȯ


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
	HR_T(m_pDevice->CreateDepthStencilView(pTextureDepthStencil.Get(), &descDSV, &m_pDepthStencilView));


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
	m_VertexCount = ARRAYSIZE(vertices);                  // ���� ���� ���� 


	// ���� ���� �Ӽ� ����ü
	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = sizeof(Vertex) * m_VertexCount;    // ���� ũ�� (���� ũ�� �� ���� ����)
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;          // ���� ���� �뵵
	bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU�� �а� ���� �⺻ ����
	bd.CPUAccessFlags = 0;

	// ���ۿ� �ʱ� ������ ������ ����ü
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices; // �迭 ������ �Ҵ� 

	// ���� ���� ����
	HR_T(m_pDevice->CreateBuffer(&bd, &vbData, m_pVertexBuffer.GetAddressOf()));

	// ���ؽ� ���� ����
	m_VertextBufferStride = sizeof(Vertex); // ���� �ϳ��� ũ��
	m_VertextBufferOffset = 0;              // ���� ������ (0)




	// ================================================================
	// 2. �ε��� ���� ����
	// ================================================================
	//
	// �ε���(Index) ����
	// - �ε����� �����ؼ� ����(Vertex) ���� 

	WORD indices[] =
	{
		3,1,0, 2,1,3,
		0,5,4, 1,5,0,
		3,4,7, 0,4,3,
		1,6,5, 2,6,1,
		2,7,6, 3,7,2,
		6,4,5, 7,4,6,
	};
	m_nIndices = ARRAYSIZE(indices);	// �ε��� ���� ����

	// �ε��� ���� �Ӽ� ����
	bd = {};
	bd.ByteWidth = sizeof(WORD) * m_nIndices;		  // ��ü �ε��� �迭 ũ��
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;           // �ε��� ���۷� ���
	bd.Usage = D3D11_USAGE_DEFAULT;                   // GPU���� �б� ����(����X)
	bd.CPUAccessFlags = 0;

	// �ε��� ������ �ʱ�ȭ ����
	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	// �ε��� ���� ����
	HR_T(m_pDevice->CreateBuffer(&bd, &ibData, m_pIndexBuffer.GetAddressOf()));





	// ================================================================
	// 3. ���ؽ� ���̴�(Vertex Shader) ������ �� ����
	// ================================================================

	ComPtr<ID3DBlob> vertexShaderBuffer; // �����ϵ� ���ؽ� ���̴� �ڵ�(hlsl) ���� ����
	
	// ' HLSL ���Ͽ��� main �Լ��� vs_4_0 �԰����� ������ '
	HR_T(CompileShaderFromFile(L"TransformVertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));

	// ���ؽ� ���̴� ��ü ����
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, m_pVertexShader.GetAddressOf()));



	// ================================================================
	// 4. �Է� ���̾ƿ�(Input Layout) ����
	// ================================================================

	// ���� ���̴��� �Է����� ���� ������ ���� ����
	// { SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate }

	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// ���ؽ� ���̴��� Input�ñ״�ó�� ���� ��ȿ�� �˻� �� -> InputLayout ����
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));



	// ================================================================
	// 5. �ȼ� ���̴�(Pixel Shader) ������ �� ����
	// ================================================================

	ComPtr<ID3DBlob> pixelShaderBuffer; // �����ϵ� ���ؽ� �ȼ� �ڵ�(hlsl) ���� ����

	// ' HLSL ���Ͽ��� main �Լ��� ps_4_0 �԰����� ������ '
	HR_T(CompileShaderFromFile(L"TransformPixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));

	// �ȼ� ���̴� ��ü ����
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf()));



	// ================================================================
	// 6.  Render() ���� ���������ο� ���ε��� ��� ���� ����
	// ================================================================

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(ConstantBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	bd.CPUAccessFlags = 0;
	HR_T(m_pDevice->CreateBuffer(&bd, nullptr, &m_pConstantBuffer));



	// [ ���̴��� ������ ������ ���� ]
	
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
