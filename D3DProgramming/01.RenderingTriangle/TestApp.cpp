#include "TestApp.h"
#include "../Common/Helper.h"

#include <directxtk/simplemath.h>
#include <d3dcompiler.h>

#pragma comment (lib, "d3d11.lib")
#pragma comment(lib,"d3dcompiler.lib")

using namespace DirectX::SimpleMath;


// [ ���� ���� ]
struct Vertex
{
	Vector3 position; // ��ġ ����
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



// �� [ ������ ] ���� : (�ʱ�ȭ �� ���������� ���� �� �׸��� �� ����ü�� ��ü) 
void TestApp::Render()
{
	// 1. ȭ�� ĥ�ϱ�
	float color[4] = { 0.80f, 0.92f, 1.0f, 1.0f }; //  Light Sky Blue 
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);


	// 2. ������ ���������� ����
	// ( Draw�迭 �Լ� ȣ�� �� -> ������ ���������ο� �ʼ� �������� ���� �ؾ��� )	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // ������ �̾ �׸� ��� (�ﰢ�� ����Ʈ ���)
	m_pDeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &m_VertextBufferStride, &m_VertextBufferOffset); // (Stride: ���� ũ��, Offset: ���� ��ġ)
	m_pDeviceContext->IASetInputLayout(m_pInputLayout);	// �Է� ���̾ƿ� ���� 
	m_pDeviceContext->VSSetShader(m_pVertexShader, nullptr, 0);		
	m_pDeviceContext->PSSetShader(m_pPixelShader, nullptr, 0);


	// 3. �׸��� 
	m_pDeviceContext->Draw(m_VertexCount, 0);


	// 4. ����ü�� ��ü (ȭ�� ��� : �� ���� <-> ����Ʈ ���� ��ü)
	m_pSwapChain->Present(0, 0);
}



// �� [ Direct3D11 �ʱ�ȭ ] : (����ü�� ���� -> ����Ÿ�ٺ� ���� -> ����Ʈ ����)
bool TestApp::InitD3D()
{
	
	HRESULT hr = 0; // ����� : DirectX �Լ��� HRESULT ��ȯ



	// =====================================
	// 1. ����ü��(Swap Chain) ����
	// =====================================

	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	swapDesc.BufferCount = 1;                                    // ����� ���� (���� ���۸�: 1)
	swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;      // ������� �뵵: ����Ÿ�� ���
	swapDesc.OutputWindow = m_hWnd;                              // ����� ������ �ڵ�
	swapDesc.Windowed = true;                                    // â ���(true) / ��üȭ��(false)
	swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;     // ����� ����: 32��Ʈ RGBA

	// �����(�ؽ�ó) �ػ� ����
	swapDesc.BufferDesc.Width = m_ClientWidth;
	swapDesc.BufferDesc.Height = m_ClientHeight;

	// ȭ�� �ֻ���(Refresh Rate) ���� (60Hz)
	swapDesc.BufferDesc.RefreshRate.Numerator = 60;
	swapDesc.BufferDesc.RefreshRate.Denominator = 1;

	// ��Ƽ���ø�(��Ƽ���ϸ����) ���� (�⺻: 1, �� ��)
	swapDesc.SampleDesc.Count = 1;
	swapDesc.SampleDesc.Quality = 0;




	// =====================================
	// 2. ����̽� / ����ü�� / ����̽� ���ؽ�Ʈ ����
	// =====================================

	UINT creationFlags = 0;

#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG; // ����� ���̾� Ȱ��ȭ (DirectX API ȣ�� �� ���� �޽��� ���)
#endif

	HR_T(D3D11CreateDeviceAndSwapChain(
		NULL,                           // �⺻ ����� ���
		D3D_DRIVER_TYPE_HARDWARE,       // �ϵ���� ������ ���
		NULL,                           // ����Ʈ���� ����̹� ����
		creationFlags,                  // ����� �÷���
		NULL, NULL,                     // �⺻ Feature Level ���
		D3D11_SDK_VERSION,              // SDK ����
		&swapDesc,                      // ����ü�� ���� ����ü
		&m_pSwapChain,                  // ����ü�� ��ȯ
		&m_pDevice,                     // ����̽� ��ȯ
		NULL,                           // Feature Level ��ȯ (��� �� ��)
		&m_pDeviceContext));            // ����̽� ���ؽ�Ʈ ��ȯ




	// =====================================
    // 3. ����Ÿ�ٺ�(Render Target View) ����
    // =====================================
	
    // ����ü���� 0�� ����(�����)�� ������
	ID3D11Texture2D* pBackBufferTexture = nullptr;
	HR_T(m_pSwapChain->GetBuffer(
		0, __uuidof(ID3D11Texture2D), (void**)&pBackBufferTexture));

	// ����� �ؽ�ó�� ������� ����Ÿ�ٺ� ����
	HR_T(m_pDevice->CreateRenderTargetView(
		pBackBufferTexture, NULL, &m_pRenderTargetView));  // �ؽ�ó�� ���� ���� ����

	// �ؽ�ó ���� ���� (����Ÿ�ٺ䰡 ���� ��� �����Ƿ� ���⼱ ���� ����) // �ܺ� ���� ī��Ʈ�� ���ҽ�Ų��.
	SAFE_RELEASE(pBackBufferTexture);	

	// ��� ����(OM: Output Merger) �ܰ迡 ����Ÿ�� ���ε�
	m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, NULL);




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



	return true;
}

void TestApp::UninitD3D()
{
	SAFE_RELEASE(m_pRenderTargetView);
	SAFE_RELEASE(m_pDeviceContext);
	SAFE_RELEASE(m_pSwapChain);
	SAFE_RELEASE(m_pDevice);
}



// �� [ Scene �ʱ�ȭ ] : (Vertex ���� ���� -> Vertex ���̴� ������ �� ���� -> ��ǲ ���̾ƿ� ���� -> Pixel ���̴� ������ �� ����)
bool TestApp::InitScene()
{

	HRESULT hr = 0;                       // DirectX �Լ� �����
	ID3D10Blob* errorMessage = nullptr;   // ���̴� ������ ���� �޽��� ����	


	// ================================================================
	// 1. ����(Vertex) ������ �غ� �� ���� ���� ����
	// ================================================================
	// 
	// ����� ����/��/�������� ��ȯ�� ���� �ʰ�,
	// ���� NDC(Normalized Device Coordinate, -1 ~ +1 ��ǥ��)�� �°� �ۼ�
	// 
	//  - ȭ�� �߾��� (0,0,0), ���� ���� (-1,1,0), ������ �Ʒ��� (1,-1,0)
	//  - �ﰢ�� �ϳ��� �����ϱ� ���� v0, v1, v2 ����
	// 
	//      /---------------------(1,1,1)   z���� ���̰�
	//     /                      / |   
	// (-1,1,0)----------------(1,1,0)        
	//   |         v1           |   |
	//   |        /   `         |   |       �߾��� (0,0,0)  
	//   |       /  +   `       |   |
	//   |     /         `      |   |
	//	 |   v0-----------v2    |  /
	// (-1,-1,0)-------------(1,-1,0)

	Vertex vertices[] =
	{
		Vector3(-0.5, -0.5, 0.5), // v0: ���� �Ʒ�
		Vector3(0.0,  0.5, 0.5), // v1: ���� �߾�
		Vector3(0.5, -0.5, 0.5), // v2: ������ �Ʒ�	
	};


	// ���� ���� �Ӽ� ����ü
	D3D11_BUFFER_DESC vbDesc = {};
	m_VertexCount = ARRAYSIZE(vertices);                  // ���� ����
	vbDesc.ByteWidth = sizeof(Vertex) * m_VertexCount;    // ���� ũ�� (���� ũ�� �� ���� ����)
	vbDesc.CPUAccessFlags = 0;                            // CPU ����X
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;          // ���� ���� �뵵
	vbDesc.MiscFlags = 0;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;                   // GPU�� �а� ���� �⺻ ����

	// ���ۿ� �ʱ� ������ ������ ����ü
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices; // ���ؽ� �迭 �ּ�

	// ���� ���� ����
	HR_T(hr = m_pDevice->CreateBuffer(&vbDesc, &vbData, &m_pVertexBuffer));

	// ���ؽ� ���� ����
	m_VertextBufferStride = sizeof(Vertex); // ���� �ϳ��� ũ��
	m_VertextBufferOffset = 0;              // ���� ������ (0)




	// ================================================================
	// 2. ���ؽ� ���̴�(Vertex Shader) ������ �� ����
	// ================================================================

	ID3DBlob* vertexShaderBuffer = nullptr; // �����ϵ� ���ؽ� ���̴� �ڵ�(hlsl) ���� ����

	// ' HLSL ���Ͽ��� main �Լ��� vs_4_0 �԰����� ������ '
	HR_T(CompileShaderFromFile(L"BasicVertexShader.hlsl", "main", "vs_4_0", &vertexShaderBuffer));

	// ���ؽ� ���̴� ��ü ����
	HR_T(m_pDevice->CreateVertexShader(
		vertexShaderBuffer->GetBufferPointer(), // �ʿ��� �����͸� �����ϸ� ��ü ���� 
		vertexShaderBuffer->GetBufferSize(), NULL, &m_pVertexShader));




	// ================================================================
	// 3. �Է� ���̾ƿ�(Input Layout) ����
	// ================================================================

	// ���� ���̴��� �Է����� ���� ������ ���� ����
	// { SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate }

	D3D11_INPUT_ELEMENT_DESC layout[] =  
	{	
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 } // ' POSITION: float3 (R32G32B32_FLOAT), ���� 0��, ���� �� ������ '
	};

	// ���ؽ� ���̴��� Input�ñ״�ó�� ���� ��ȿ�� �˻� �� -> InputLayout ����
	HR_T(hr = m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout),
		vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_pInputLayout));

	// ���������� ���۴� ���� ����
	SAFE_RELEASE(vertexShaderBuffer); 




	// ================================================================
	// 4. �ȼ� ���̴�(Pixel Shader) ������ �� ����
	// ================================================================

	ID3DBlob* pixelShaderBuffer = nullptr; // �����ϵ� ���ؽ� �ȼ� �ڵ�(hlsl) ���� ����

	// ' HLSL ���Ͽ��� main �Լ��� ps_4_0 �԰����� ������ '
	HR_T(CompileShaderFromFile(L"BasicPixelShader.hlsl", "main", "ps_4_0", &pixelShaderBuffer));

	// �ȼ� ���̴� ��ü ����
	HR_T(m_pDevice->CreatePixelShader(	  // �ʿ��� �����͸� �����ϸ� ��ü ���� 
		pixelShaderBuffer->GetBufferPointer(),
		pixelShaderBuffer->GetBufferSize(), NULL, &m_pPixelShader));

	// ���������� ���۴� ���� ����
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
