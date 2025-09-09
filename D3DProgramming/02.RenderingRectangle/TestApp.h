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

	// [ 렌더링 파이프라인을 구성하는 필수 객체의 인터페이스 ] ( 뎊스 스텐실 뷰도 있지만 아직 사용X )
	ComPtr<ID3D11Device> m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDeviceContext;
	ComPtr<IDXGISwapChain> m_pSwapChain;
	ComPtr<ID3D11RenderTargetView> m_pRenderTargetView;

	// [ 렌더링 파이프라인 객체 ]
	ComPtr<ID3D11VertexShader> m_pVertexShader; // 정점 셰이더
	ComPtr<ID3D11PixelShader> m_pPixelShader;	// 픽셀 셰이더
	ComPtr<ID3D11InputLayout> m_pInputLayout;	// 입력 레이아웃
	ComPtr<ID3D11Buffer> m_pVertexBuffer;		// 버텍스 버퍼
	ComPtr<ID3D11Buffer> m_pIndexBuffer;		// 인덱스 버퍼

	// [ 렌더링 파이프라인관련 정보 ]
	UINT m_VertextBufferStride = 0;					// 버텍스 하나의 크기
	UINT m_VertextBufferOffset = 0;					// 버텍스 버퍼의 오프셋
	UINT m_VertexCount = 0;							// 버텍스 개수
	int m_nIndices = 0;								// 인덱스 개수

	virtual bool Initialize();	
	virtual void Update();
	virtual void Render();

	bool InitD3D();
	void UninitD3D();		

	bool InitScene();		// 쉐이더,버텍스,인덱스
	void UninitScene();
};