#pragma once
#include <windows.h>
#include "TimeSystem.h"
#include "InputSystem.h"
#include "Camera.h"

#define MAX_LOADSTRING 100


class GameApp : public InputProcesser
{
public:
	GameApp();
	virtual ~GameApp();

	static HWND m_hWnd;						// ���� �ʿ��ϴ� ������ ���������� ���ϱ� ���� ��������� 
	static GameApp* m_pInstance;			// �����ڿ��� �ν��Ͻ� �����͸� ����

public:
	HACCEL m_hAccelTable;
	MSG m_msg;
	HINSTANCE m_hInstance;                            // ���� �ν��Ͻ�
	WCHAR m_szTitle[MAX_LOADSTRING];                  // ���� ǥ���� �ؽ�Ʈ
	WCHAR m_szWindowClass[MAX_LOADSTRING];            // �⺻ â Ŭ���� �̸�
	WNDCLASSEXW m_wcex;
	float m_previousTime;
	float m_currentTime;
	int  m_nCmdShow;
	TimeSystem m_Timer;
	InputSystem m_Input;
	Camera m_Camera;
	UINT m_ClientWidth;
	UINT m_ClientHeight;

public:
	// ������ ���� ���,����,���̱�
	virtual bool Initialize();
	virtual void Uninitialize() {};
	virtual bool Run(HINSTANCE hInstance);
	virtual void Update();		// ��� ���� Ŭ�������� ����
	virtual void Render();		// ��� ���� Ŭ�������� ����

	virtual void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker);

	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void SetClientSize(UINT width, UINT height) { m_ClientWidth = width; m_ClientHeight = height; }
};

