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

	static HWND m_hWnd;						// 자주 필요하니 포인터 간접접근을 피하기 위해 정적멤버로 
	static GameApp* m_pInstance;			// 생성자에서 인스턴스 포인터를 보관

public:
	HACCEL m_hAccelTable;
	MSG m_msg;
	HINSTANCE m_hInstance;                            // 현재 인스턴스
	WCHAR m_szTitle[MAX_LOADSTRING];                  // 제목 표시줄 텍스트
	WCHAR m_szWindowClass[MAX_LOADSTRING];            // 기본 창 클래스 이름
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
	// 윈도우 정보 등록,생성,보이기
	virtual bool Initialize();
	virtual void Uninitialize() {};
	virtual bool Run(HINSTANCE hInstance);
	virtual void Update();		// 상속 받은 클래스에서 구현
	virtual void Render();		// 상속 받은 클래스에서 구현

	virtual void OnInputProcess(const Keyboard::State& KeyState, const Keyboard::KeyboardStateTracker& KeyTracker,
		const Mouse::State& MouseState, const Mouse::ButtonStateTracker& MouseTracker);

	virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	void SetClientSize(UINT width, UINT height) { m_ClientWidth = width; m_ClientHeight = height; }
};

