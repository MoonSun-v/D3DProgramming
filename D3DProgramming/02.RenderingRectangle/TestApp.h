#pragma once
#include <d3d11.h>
#include <wrl/client.h>

#include "../Common/GameApp.h"

using Microsoft::WRL::ComPtr;



class TestApp : public GameApp
{
public:
	TestApp();
	~TestApp();

	// [ ������ ������������ �����ϴ� �ʼ� ��ü�� �������̽� ] ( �X�� ���ٽ� �䵵 ������ ���� ���X )
	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	ComPtr<IDXGISwapChain> m_pSwapChain;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;

	// [ ������ ���������� ��ü ]
	ComPtr<ID3D11VertexShader> m_pVertexShader; // ���� ���̴�
	ComPtr<ID3D11PixelShader> m_pPixelShader;	// �ȼ� ���̴�
	ComPtr<ID3D11InputLayout> m_pInputLayout;	// �Է� ���̾ƿ�
	ComPtr<ID3D11Buffer> m_pVertexBuffer;		// ���ؽ� ����
	ComPtr<ID3D11Buffer> m_pIndexBuffer;		// �ε��� ����

	// [ ������ ���������ΰ��� ���� ]
	UINT m_VertextBufferStride = 0;					// ���ؽ� �ϳ��� ũ��
	UINT m_VertextBufferOffset = 0;					// ���ؽ� ������ ������
	UINT m_VertexCount = 0;							// ���ؽ� ����
	int m_nIndices = 0;								// �ε��� ����

	virtual bool Initialize();	
	virtual void Update();
	virtual void Render();

	bool InitD3D();
	void UninitD3D();		

	bool InitScene();		// ���̴�,���ؽ�,�ε���
	void UninitScene();
};