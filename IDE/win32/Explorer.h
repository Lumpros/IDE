#pragma once

#include "Window.h"
#include "WorkArea.h"
#include "StatusBar.h"

#include <string>
#include <CommCtrl.h>

class Explorer : public Window
{
public:
	Explorer(HWND hParentWindow);
	~Explorer(void);

	void SetStatusBar(StatusBar* m_pStatusBar);
	void CloseProjectFolder(void);
	void OpenProjectFolder(std::wstring folder);
	void SaveCurrentFile(WorkArea* pWorkArea);
	void SaveAllFiles(WorkArea* pWorkArea);
	HWND GetTreeHandle(void) const;

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	HRESULT InitializeWindow(HINSTANCE hInstance);

	LRESULT OnGetMinMaxInfo(HWND hWnd, LPARAM lParam);
	LRESULT OnNCHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam);
	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnNotify(HWND hWnd, LPARAM lParam);
	LRESULT OnPaint(HWND hWnd);
	LRESULT OnCommand(HWND hWnd, WPARAM wParam);

	void OnOpenInFileExplorer(void);
	void OnRename(void);

	void OnNMDoubleClick(void);
	void OnContextOpen(void);
	void OnRClickCreateContextMenu(void);
	void OnDelete(void);
	void InitializeImageList(void);

	LRESULT OnEndLabelEdit(
		LPARAM lParam
	);

	void OnFolderNameChange(
		const std::wstring& absolute_path,
		const std::wstring& newabsolute_path,
		WorkArea* pWorkArea
	);

	void UpdateOpenedTabName(
		const std::wstring& absolute_path,
		const std::wstring& new_absolute_path,
		WorkArea* pWorkArea
	);

	void ChangeSavedAbsolutePath(
		const std::wstring& absolute_path,
		const std::wstring& new_absolute_path,
		const bool is_directory
	);

	void ExploreDirectory(
		const wchar_t* directory,
		HTREEITEM hParent
	);

	void OpenTabFromFilePath(
		LPCWSTR lpFilePath
	);

	void SaveFileFromTab(
		SourceTab* pSourceTab
	);

	HTREEITEM GetClickedTreeItemPath(
		_In_  HWND hTreeWindow,
		_In_  POINT ptClick,
		_Out_ std::wstring& path
	);

	HTREEITEM GetItemPath(
		_In_ HWND hTreeWindow,
		_In_ HTREEITEM hItem,
		_Out_ std::wstring& path
	);

private:
	RECT m_rcTree = { 0 };
	HWND m_hTreeWindow = nullptr;
	HWND m_hRenameEdit = nullptr;
	HIMAGELIST hImageList = nullptr;
	HICON hFileIcon = nullptr;
	HTREEITEM m_hRightClickedItem = nullptr;
	StatusBar* m_pStatusBar = nullptr;

	// The directory in which the root folder is in
	std::wstring m_RootDirectory;
};

