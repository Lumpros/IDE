#pragma once

#include "Window.h"
#include "WorkArea.h"
#include "StatusBar.h"

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
	void SaveFileFromTab(SourceTab* pSourceTab);

	RECT m_rcTree = { 0 };
	HWND m_hTreeWindow = nullptr;
	HIMAGELIST hImageList = nullptr;
	HICON hFileIcon = nullptr;
	HTREEITEM m_hRightClickedItem = nullptr;
	StatusBar* m_pStatusBar = nullptr;

public:
	Explorer(HWND hParentWindow);
	~Explorer(void);

	void SetStatusBar(StatusBar* m_pStatusBar);

	void CloseProjectFolder(void);
	void OpenProjectFolder(std::wstring folder);

	void SaveCurrentFile(WorkArea* pWorkArea);
	void SaveAllFiles(WorkArea* pWorkArea);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	HWND GetTreeHandle(void) const;
};

