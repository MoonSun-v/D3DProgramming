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
	Vector3 position;	// ��ġ ����
	Vector4 color;		// ���� ����

	Vertex(float x, float y, float z) : position(x, y, z) {}
	Vertex(Vector3 position) : position(position) {}

	Vertex(Vector3 position, Vector4 color): position(position), color(color) {}
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

void TestApp::Update()
{

}



// �� [ ������ ] ���� : (�ʱ�ȭ �� ���������� ���� �� �׸��� �� ����ü�� ��ü) 
void TestApp::Render()
{
	// 1. ȭ�� ĥ�ϱ�
	float color[4] = { 0.80f, 0.92f, 1.0f, 1.0f }; //  Light Sky Blue 
	m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView.Get(), color);


	// 2. ������ ���������� ����
	// ( Draw�迭 �Լ� ȣ�� �� -> ������ ���������ο� �ʼ� �������� ���� �ؾ��� )	
	m_pDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST); // ������ �̾ �׸� ��� (�ﰢ�� ����Ʈ ���)
	m_pDeviceContext->IASetVertexBuffers(0, 1, m_pVertexBuffer.GetAddressOf(), &m_VertextBufferStride, &m_VertextBufferOffset); // (Stride: ���� ũ��, Offset: ���� ��ġ)
	m_pDeviceContext->IASetInputLayout(m_pInputLayout.Get());
	m_pDeviceContext->IASetIndexBuffer(m_pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	m_pDeviceContext->VSSetShader(m_pVertexShader.Get(), nullptr, 0);
	m_pDeviceContext->PSSetShader(m_pPixelShader.Get(), nullptr, 0);


	// 3. �׸��� (�ε��� ���� ��� �簢�� ������)
	// �� �ε��� ���� ���� ������ ����� ��� -> Draw() ���
	m_pDeviceContext->DrawIndexed(m_nIndices, 0, 0);


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
	// ����̽�, ����ü��, ���ؽ�Ʈ ����
	HR_T(D3D11CreateDeviceAndSwapChain(
		nullptr,						// �⺻ ����� ���
		D3D_DRIVER_TYPE_HARDWARE,		// �ϵ���� ������ ���
		nullptr,						// ����Ʈ���� ����̹� ����
		creationFlags,					// ����� �÷���
		nullptr, 0,						// �⺻ Feature Level ���
		D3D11_SDK_VERSION,				// SDK ����
		&swapDesc,						// ����ü�� ���� ����ü
		m_pSwapChain.GetAddressOf(),	// ����ü�� ��ȯ
		m_pDevice.GetAddressOf(),		// ����̽� ��ȯ
		nullptr,						// Feature Level ��ȯ (��� �� ��)
		m_pDeviceContext.GetAddressOf()	// ����̽� ���ؽ�Ʈ ��ȯ
	));



	// =====================================
    // 3. ����Ÿ�ٺ�(Render Target View) ����
    // =====================================
	
    // ����ü���� 0�� ����(�����)�� ������
	ComPtr<ID3D11Texture2D> pBackBufferTexture;

	HR_T(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(pBackBufferTexture.GetAddressOf())));

	// ����� �ؽ�ó�� ������� ����Ÿ�ٺ� ����
	HR_T(m_pDevice->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, m_pRenderTargetView.GetAddressOf()));

	// ��� ����(OM: Output Merger) �ܰ迡 ����Ÿ�� ���ε�
	m_pDeviceContext->OMSetRenderTargets(1, m_pRenderTargetView.GetAddressOf(), nullptr);





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
	// 4���� ������ ����� �簢��(Rectangle)�� �����.
	// �簢���� �� ���� �ﰢ��(0,1,2) + (2,1,3)���� ����
	//
	// 
	//   v0(-0.5,+0.5) ---- v1(+0.5,+0.5)
	//        |                 |
	//        |                 |
	//   v2(-0.5,-0.5) ---- v3(+0.5,-0.5)

	Vertex vertices[] =
	{
		Vertex(Vector3(-0.5f,  0.5f, 0.5f), Vector4(1.0f, 0.0f, 0.0f, 1.0f)),  // ���� (v0)
		Vertex(Vector3(0.5f,  0.5f, 0.5f), Vector4(0.0f, 1.0f, 0.0f, 1.0f)),   // �ʷ� (v1)
		Vertex(Vector3(-0.5f, -0.5f, 0.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f)),  // �Ķ� (v2)
		Vertex(Vector3(0.5f, -0.5f, 0.5f), Vector4(0.0f, 0.0f, 1.0f, 1.0f))    // �Ķ� (v3)
	};
	m_VertexCount = ARRAYSIZE(vertices);                  // ���� ���� ���� 


	// ���� ���� �Ӽ� ����ü
	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.ByteWidth = sizeof(Vertex) * m_VertexCount;    // ���� ũ�� (���� ũ�� �� ���� ����)
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;          // ���� ���� �뵵
	vbDesc.Usage = D3D11_USAGE_DEFAULT;                   // GPU�� �а� ���� �⺻ ����

	// ���ۿ� �ʱ� ������ ������ ����ü
	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices; // ���ؽ� �迭 �ּ�

	// ���� ���� ����
	HR_T(m_pDevice->CreateBuffer(&vbDesc, &vbData, m_pVertexBuffer.GetAddressOf()));

	// ���ؽ� ���� ����
	m_VertextBufferStride = sizeof(Vertex); // ���� �ϳ��� ũ��
	m_VertextBufferOffset = 0;              // ���� ������ (0)




	// ================================================================
	// 2. �ε��� ���� ����
	// ================================================================
	//
	// �ε���(Index) ����
	// - �ε����� �����ؼ� ����(Vertex) ���� 
	// 
	// �� �� �ﰢ�� ���ļ� �ϳ��� �簢��(Rectangle) ����� 

	WORD indices[] =
	{
		0, 1, 2,   // ù ��° �ﰢ�� (v0, v1, v2)
		2, 1, 3    // �� ��° �ﰢ�� (v2, v1, v3)
	};
	m_nIndices = ARRAYSIZE(indices);	// �ε��� ���� ����

	// �ε��� ���� �Ӽ� ����
	D3D11_BUFFER_DESC ibDesc = {};
	ibDesc.ByteWidth = sizeof(WORD) * ARRAYSIZE(indices); // ��ü �ε��� �迭 ũ��
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;           // �ε��� ���۷� ���
	ibDesc.Usage = D3D11_USAGE_DEFAULT;                   // GPU���� �б� ����(����X)

	// �ε��� ������ �ʱ�ȭ ����
	D3D11_SUBRESOURCE_DATA ibData = {};
	ibData.pSysMem = indices;

	// �ε��� ���� ����
	HR_T(m_pDevice->CreateBuffer(&ibDesc, &ibData, m_pIndexBuffer.GetAddressOf()));





	// ================================================================
	// 3. ���ؽ� ���̴�(Vertex Shader) ������ �� ����
	// ================================================================

	ComPtr<ID3DBlob> vertexShaderBuffer; // �����ϵ� ���ؽ� ���̴� �ڵ�(hlsl) ���� ����
	
	// ' HLSL ���Ͽ��� main �Լ��� vs_4_0 �԰����� ������ '
	HR_T(CompileShaderFromFile(L"RectangleVertexShader.hlsl", "main", "vs_4_0", vertexShaderBuffer.GetAddressOf()));

	// ���ؽ� ���̴� ��ü ����
	HR_T(m_pDevice->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, m_pVertexShader.GetAddressOf()));



	// ================================================================
	// 4. �Է� ���̾ƿ�(Input Layout) ����
	// ================================================================

	// ���� ���̴��� �Է����� ���� ������ ���� ����
	// { SemanticName , SemanticIndex , Format , InputSlot , AlignedByteOffset , InputSlotClass , InstanceDataStepRate }

	D3D11_INPUT_ELEMENT_DESC layout[] =  
	{	
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	// ���ؽ� ���̴��� Input�ñ״�ó�� ���� ��ȿ�� �˻� �� -> InputLayout ����
	HR_T(m_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), m_pInputLayout.GetAddressOf()));



	// ================================================================
	// 5. �ȼ� ���̴�(Pixel Shader) ������ �� ����
	// ================================================================

	ComPtr<ID3DBlob> pixelShaderBuffer; // �����ϵ� ���ؽ� �ȼ� �ڵ�(hlsl) ���� ����

	// ' HLSL ���Ͽ��� main �Լ��� ps_4_0 �԰����� ������ '
	HR_T(CompileShaderFromFile(L"RectanglePixelShader.hlsl", "main", "ps_4_0", pixelShaderBuffer.GetAddressOf()));

	// �ȼ� ���̴� ��ü ����
	HR_T(m_pDevice->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, m_pPixelShader.GetAddressOf()));



	return true;
}

void TestApp::UninitScene()
{
}
