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
	LRESULT OnPaint(HWND hWnd);
	LRESULT OnCommand(HWND hWnd, WPARAM wParam);

	void OnContextOpen(void);

	void ExploreDirectory(const wchar_t* directory, HTREEITEM hParent);
	void OnRClickCreateContextMenu(void);
	void OpenTabFromFilePath(LPCWSTR lpFilePath);

	POINT m_ptCursorOnRClick = { 0 };
	RECT m_rcTree = { 0 };
	HWND m_hTreeWindow = nullptr;
	HIMAGELIST hImageList = nullptr;
	HICON hFileIcon = nullptr;

public:
	Explorer(HWND hParentWindow);
	~Explorer(void);

	void CloseProjectFolder(void);
	void OpenProjectFolder(std::wstring folder);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

