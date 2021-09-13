#pragma once

#include "Window.h"

class Explorer : public Window
{
private:
	HRESULT InitializeWindow(HINSTANCE hInstance);
	
	LRESULT OnGetMinMaxInfo(HWND hWnd, LPARAM lParam);
	LRESULT OnNCHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(HWND hWnd, LPARAM lParam);

	RECT m_rcTree = { 0 };
	HWND m_hTreeWindow = nullptr;

public:
	Explorer(HWND hParentWindow);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

