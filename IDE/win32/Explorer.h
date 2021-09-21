#pragma once

#include "Window.h"

#include <string>
#include <CommCtrl.h>

class Explorer : public Window
{
private:
	HRESULT InitializeWindow(HINSTANCE hInstance);
	
	LRESULT OnGetMinMaxInfo(HWND hWnd, LPARAM lParam);
	LRESULT OnNCHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnNotify(HWND hWnd, LPARAM lParam);

	void ExploreDirectory(const wchar_t* directory, HTREEITEM hParent);

	RECT m_rcTree = { 0 };
	HWND m_hTreeWindow = nullptr;

public:
	Explorer(HWND hParentWindow);

	void OpenProjectFolder(std::wstring folder);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

