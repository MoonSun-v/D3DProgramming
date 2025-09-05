#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "../Common/GameApp.h"

using Microsoft::WRL::ComPtr;



class TestApp : public GameApp
{
public:
	TestApp(HINSTANCE hInstance);
	~TestApp();

	// [ ������ ������������ �����ϴ� �ʼ� ��ü�� �������̽� ] ( �X�� ���ٽ� �䵵 ������ ���� ���X )
	//ID3D11Device* m_pDevice = nullptr;						
	//ID3D11DeviceContext* m_pDeviceContext = nullptr;		
	//IDXGISwapChain* m_pSwapChain = nullptr;				
	//ID3D11RenderTargetView* m_pRenderTargetView = nullptr;	
	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	ComPtr<IDXGISwapChain> m_pSwapChain;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;

	// [ ������ ���������� ��ü ]
	//ID3D11VertexShader* m_pVertexShader = nullptr;	// ���� ���̴�
	//ID3D11PixelShader* m_pPixelShader = nullptr;	// �ȼ� ���̴�
	//ID3D11InputLayout* m_pInputLayout = nullptr;	// �Է� ���̾ƿ�
	//ID3D11Buffer* m_pVertexBuffer = nullptr;		// ���ؽ� ����
	//ID3D11Buffer* m_pIndexBuffer = nullptr;			// �ε��� ����
	ComPtr<ID3D11VertexShader> m_pVertexShader;
	ComPtr<ID3D11PixelShader> m_pPixelShader;
	ComPtr<ID3D11InputLayout> m_pInputLayout;
	ComPtr<ID3D11Buffer> m_pVertexBuffer;
	ComPtr<ID3D11Buffer> m_pIndexBuffer;

	// [ ������ ���������ΰ��� ���� ]
	UINT m_VertextBufferStride = 0;					// ���ؽ� �ϳ��� ũ��
	UINT m_VertextBufferOffset = 0;					// ���ؽ� ������ ������
	UINT m_VertexCount = 0;							// ���ؽ� ����
	int m_nIndices = 0;								// �ε��� ����

	virtual bool Initialize(UINT Width, UINT Height);	
	virtual void Update();
	virtual void Render();

	bool InitD3D();
	void UninitD3D();		// ���� Relese -> Reset() 

	bool InitScene();		// ���̴�,���ؽ�,�ε���
	void UninitScene();
};