#pragma once

#include "Window.h"
#include "WorkArea.h"
#include "StatusBar.h"
#include "FileClipboard.h"

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

	/// <summary>
	/// Sets the children of hParent as the 
	/// files and folders of the specified directory
	/// </summary>
	/// <param name="directory">: Absolute path of file/folder </param>
	/// <param name="hParent">: Handle to the parent item </param>
	void ExploreDirectory(
		const wchar_t* directory,
		HTREEITEM hParent
	);

	HTREEITEM GetItemPath(
		_In_ HWND hTreeWindow,
		_In_ HTREEITEM hItem,
		_Out_ std::wstring& path
	);

	HTREEITEM GetRightClickedItem(void) const { return m_hRightClickedItem; }
	HTREEITEM& GetItemToCut(void) { return m_hItemToCut; }

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
	void OnCopy(void);

	void InitializeImageList(void);
	HMENU CreateContextMenu(const std::wstring& path);

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

private:
	RECT m_rcTree = { 0 };
	HWND m_hTreeWindow = nullptr;
	HWND m_hRenameEdit = nullptr;
	HIMAGELIST hImageList = nullptr;
	HICON hFileIcon = nullptr;
	HTREEITEM m_hRightClickedItem = nullptr;
	HTREEITEM m_hItemToCut = nullptr;
	StatusBar* m_pStatusBar = nullptr;
	FileClipboard m_Clipboard;

	// The directory in which the root folder is in
	std::wstring m_RootDirectory;
};

