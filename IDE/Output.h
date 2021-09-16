#pragma once

#include "Window.h"
#include "ODButton.h"

class Output : public Window
{
private:
	HRESULT InitializeOutputWindow(HINSTANCE hInstance);

	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnNCHitTest(HWND hWnd);
	LRESULT OnWindowPosChanged(HWND hWnd, LPARAM lParam);
	LRESULT OnPaint(HWND hWnd);
	LRESULT OnGetMinMaxInfo(HWND hWnd, LPARAM lParam);

	int GetMinimumHeight(void);

	ODButton* m_pOutputButton = nullptr;
	ODButton* m_pErrorListButton = nullptr;

public:
	explicit Output(HWND hWnd);
	~Output(void);

	LRESULT WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

