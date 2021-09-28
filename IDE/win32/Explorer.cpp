#include "Explorer.h"
#include "WorkArea.h"
#include "Logger.h"
#include "Utility.h"
#include "AppWindow.h"
#include "resource.h"

#include <shellapi.h>
#include <CommCtrl.h>
#include <windowsx.h>
#include <fstream>
#include <ShlObj.h>
#include <atlbase.h>

#define EXPLORER_WINDOW_CLASS L"IDEExplorerWindowClass"

#define HTRIGHT_WIDTH 3

#define IDC_CONTEXT_OPEN 3000
#define IDC_CONTEXT_RENAME 3001
#define IDC_CONTEXT_COPY 3002
#define IDC_CONTEXT_CUT 3003
#define IDC_CONTEXT_DELETE 3004
#define IDC_CONTEXT_OPEN_IN_FILE_EXPLORER 3005
#define IDC_CONTEXT_PASTE 3006
#define IDC_CONTEXT_NEW_FILE 3007
#define IDC_CONTEXT_NEW_FOLDER 3008

static HRESULT RegisterExplorerWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK ExplorerWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

struct TABINFO {
	SourceTab* pSourceTab = nullptr;
	TabList* pTabList = nullptr;
	int index = 0;
};

Explorer::Explorer(HWND hParentWindow)
{
	const HINSTANCE hInstance = GetModuleHandle(NULL);

	this->m_hWndParent = hParentWindow;

	if (FAILED(RegisterExplorerWindowClass(hInstance)))
	{
		Logger::Write(L"Failed to register file explorer window class!");
		PostQuitMessage(1);
		return;
	}

	if (FAILED(InitializeWindow(hInstance)))
	{
		Logger::Write(L"Failed to create file explorer window!");
		PostQuitMessage(1);
		return;
	}
}

void Explorer::SetStatusBar(StatusBar* pStatusBar)
{
	m_pStatusBar = pStatusBar;
}

static HRESULT RegisterExplorerWindowClass(HINSTANCE hInstance)
{
	static bool hasBeenRegistered = false;

	if (!hasBeenRegistered)
	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(WNDCLASSEX));
		wcex.cbSize = sizeof(wcex);
		wcex.lpszClassName = EXPLORER_WINDOW_CLASS;
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpfnWndProc = ::ExplorerWindowProcedure;
		wcex.cbWndExtra = sizeof(Explorer*);

		if (!RegisterClassEx(&wcex))
			return E_FAIL;

		hasBeenRegistered = true;
	}

	return S_OK;
}

HRESULT Explorer::InitializeWindow(HINSTANCE hInstance)
{
	m_hWndSelf = CreateWindowEx(
		NULL,
		EXPLORER_WINDOW_CLASS,
		NULL,
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		m_hWndParent,
		nullptr,
		hInstance,
		this
	);

	if (m_hWndSelf)
	{
		m_hTreeWindow = CreateWindowEx(
			NULL,
			WC_TREEVIEW,
			L"Tree View",
			WS_VISIBLE | WS_CHILD | TVS_EDITLABELS |
			TVS_HASBUTTONS | WS_CLIPCHILDREN,
			0, 0,
			m_rcSelf.right,
			m_rcSelf.bottom,
			m_hWndSelf,
			nullptr,
			hInstance,
			nullptr
		);

		if (!m_hTreeWindow) {
			Logger::Write(L"Failed to create tree view window!");
			return E_FAIL;
		}

		InitializeImageList();
	}

	return m_hWndSelf ? S_OK : E_FAIL;
}

Explorer::~Explorer(void)
{
	ImageList_Destroy(hImageList);

	SAFE_DELETE_GDIOBJ(hFileIcon);
}

void Explorer::InitializeImageList(void)
{
	if (hImageList)
		ImageList_Destroy(hImageList);

	const int size = static_cast<int>(Utility::GetScaleForDPI(m_hWndParent) * 17);

	hImageList = ImageList_Create(size, size, ILC_COLOR32, 2, 0);
	
	SHSTOCKICONINFO sInfo = {};
	sInfo.cbSize = sizeof(sInfo);
	SHGetStockIconInfo(SIID_FOLDEROPEN, SHGSI_ICON, &sInfo);

	if (sInfo.hIcon != nullptr)
	{
		ImageList_AddIcon(hImageList, sInfo.hIcon);
	}

	SHGetStockIconInfo(SIID_DOCNOASSOC, SHGSI_ICON, &sInfo);

	if (sInfo.hIcon != nullptr)
	{
		ImageList_AddIcon(hImageList, sInfo.hIcon);
	}

	TreeView_SetImageList(m_hTreeWindow, hImageList, TVSIL_NORMAL);
}

void Explorer::CloseProjectFolder(void)
{
	TreeView_DeleteAllItems(m_hTreeWindow);
}

HTREEITEM Explorer::GetClickedTreeItemPath(
	_In_  HWND hTreeWindow,
	_In_  POINT ptClick,
	_Out_ std::wstring& path
)
{
	TVHITTESTINFO htInfo = {};
	htInfo.pt = ptClick;

	return GetItemPath(hTreeWindow, TreeView_HitTest(hTreeWindow, &htInfo), path);
}

HTREEITEM Explorer::GetItemPath(
	_In_ HWND hTreeWindow,
	_In_ HTREEITEM hItem,
	_Out_ std::wstring& path
)
{
	path.clear();

	wchar_t file_name[128];
	wchar_t buf[128];

	TVITEM item;
	item.mask = TVIF_TEXT;
	item.hItem = hItem;
	item.cchTextMax = 128;
	item.pszText = file_name;
	TreeView_GetItem(hTreeWindow, &item);

	item.pszText = buf;
	HTREEITEM hParentItem = TreeView_GetParent(hTreeWindow, item.hItem);

	while (hParentItem != nullptr)
	{
		std::wstring new_str = L"\\";
		item.hItem = hParentItem;
		item.mask = TVIF_TEXT;
		TreeView_GetItem(hTreeWindow, &item);

		new_str.insert(0, item.pszText);

		path.insert(0, new_str);

		hParentItem = TreeView_GetParent(hTreeWindow, hParentItem);
	}

	path.insert(0, m_RootDirectory);
	path.append(file_name);

	return hItem;
}

LRESULT Explorer::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ERASEBKGND:
		return FALSE;

	case WM_PAINT:
		return OnPaint(hWnd);

	case WM_NCHITTEST:
		return OnNCHitTest(hWnd, wParam, lParam);

	case WM_GETMINMAXINFO:
		return OnGetMinMaxInfo(hWnd, lParam);

	case WM_SIZE:
		return OnSize(hWnd, lParam);

	case WM_NOTIFY:
		return OnNotify(hWnd, lParam);

	case WM_COMMAND:
		return OnCommand(hWnd, wParam);

	case WM_DPICHANGED_BEFOREPARENT:
		InitializeImageList();
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Explorer::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

	RECT rcNew = {};
	rcNew.left = m_rcSelf.right - HTRIGHT_WIDTH - 1;
	rcNew.top = 0;
	rcNew.right = m_rcSelf.right;
	rcNew.bottom = m_rcSelf.bottom;

	FillRect(hDC, &rcNew, (HBRUSH)COLOR_WINDOW);

	EndPaint(hWnd, &ps);
	return 0;
}

LRESULT Explorer::OnNotify(HWND hWnd, LPARAM lParam)
{
	LPNMHDR info = reinterpret_cast<LPNMHDR>(lParam);

	switch (info->code)
	{
	case NM_DBLCLK:
		OnNMDoubleClick();
		break;

	case NM_RCLICK:
		OnRClickCreateContextMenu();
		break;

	case TVN_ENDLABELEDIT:
		return OnEndLabelEdit(lParam);
	}

	return 0;
}

static bool FileNameContainsInvalidCharacters(LPCWSTR name)
{
	constexpr wchar_t invalid_chars[] = L"\\/:*?\"<>|";

	for (size_t i = 0, len = lstrlen(name); i < len; ++i)
	{
		for (size_t k = 0; k < sizeof(invalid_chars) / sizeof(wchar_t); ++k)
		{
			if (name[i] == invalid_chars[k])
			{
				return true;
			}
		}
	}
	
	return false;
}

LRESULT Explorer::OnEndLabelEdit(LPARAM lParam)
{
	LPNMTVDISPINFO pInfo = reinterpret_cast<LPNMTVDISPINFO>(lParam);

	// Item editing wasn't cancelled
	if (pInfo->item.pszText != nullptr) 
	{
		if (!FileNameContainsInvalidCharacters(pInfo->item.pszText))
		{
			std::wstring absolute_path;
			GetItemPath(m_hTreeWindow, pInfo->item.hItem, absolute_path);

			std::wstring new_absolute_path = absolute_path.substr(0, absolute_path.find_last_of(L'\\') + 1) + pInfo->item.pszText;
		
			const bool is_directory = Utility::IsPathDirectory(absolute_path);

			if (MoveFile(absolute_path.c_str(), new_absolute_path.c_str()))
			{
				ChangeSavedAbsolutePath(absolute_path, new_absolute_path, is_directory);

				return TRUE;
			}

			else
			{
				m_pStatusBar->SetText(L"Unable to rename file.", 0);
			}
		}

		else
		{
			m_pStatusBar->SetText(L"Invalid character entered in name.", 0);
		}
	}

	return FALSE;
}

void Explorer::ChangeSavedAbsolutePath(
	const std::wstring& absolute_path,
	const std::wstring& new_absolute_path,
	const bool is_directory
)
{
	AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(m_hWndParent);

	if (pAppWindow != nullptr)
	{
		WorkArea* pWorkArea = pAppWindow->GetWorkArea();

		if (pWorkArea != nullptr)
		{
			if (is_directory)
			{
				OnFolderNameChange(absolute_path, new_absolute_path, pWorkArea);
			}

			else
			{
				UpdateOpenedTabName(absolute_path, new_absolute_path, pWorkArea);
			}
		}
	}
}

static std::wstring GetFileNameFromPath(const std::wstring& path)
{
	return path.substr(path.find_last_of(L'\\') + 1, std::wstring::npos);
}

void Explorer::OnFolderNameChange(
	const std::wstring& absolute_path,
	const std::wstring& new_absolute_path,
	WorkArea* pWorkArea
)
{
	TabList& open_tabs = pWorkArea->GetVisibleTabs();
	TabList& close_tabs = pWorkArea->GetHiddenTabs();

	const size_t path_len = absolute_path.size();

	for (SourceTab* pSourceTab : open_tabs) 
	{
		if (wcsstr(pSourceTab->GetPath(), absolute_path.c_str()) != nullptr)
		{
			std::wstring file_path = pSourceTab->GetPath();
			file_path.replace(file_path.begin(), file_path.begin() + path_len, new_absolute_path);
			pSourceTab->SetName(file_path.c_str());
		}
	}

	for (SourceTab* pSourceTab : close_tabs)
	{
		if (wcsstr(pSourceTab->GetPath(), absolute_path.c_str()) != nullptr)
		{
			std::wstring file_path = pSourceTab->GetPath();
			file_path.replace(file_path.begin(), file_path.begin() + path_len, new_absolute_path);
			pSourceTab->SetName(file_path.c_str());
		}
	}
}

void Explorer::UpdateOpenedTabName(
	const std::wstring& absolute_path,
	const std::wstring& new_absolute_path,
	WorkArea* pWorkArea
)
{
	TabList& opened_tabs = pWorkArea->GetVisibleTabs();

	for (SourceTab* pOpenTab : opened_tabs)
	{
		if (pOpenTab->GetPath() == absolute_path)
		{
			pOpenTab->SetName(new_absolute_path.c_str());
			return;
		}
	}

	TabList& closed_tabs = pWorkArea->GetHiddenTabs();

	for (SourceTab* pCloseTab : opened_tabs)
	{
		if (pCloseTab->GetPath() == absolute_path)
		{
			pCloseTab->SetName(new_absolute_path.c_str());
		}
	}
}

void Explorer::OnNMDoubleClick(void)
{
	POINT ptCursor;
	GetCursorPos(&ptCursor);
	ScreenToClient(m_hTreeWindow, &ptCursor);

	std::wstring selection_path;
	GetClickedTreeItemPath(m_hTreeWindow, ptCursor, selection_path);

	if (!selection_path.empty() && !Utility::IsPathDirectory(selection_path))
	{
		OpenTabFromFilePath(selection_path.c_str());
	}
}

void Explorer::OnRClickCreateContextMenu(void)
{
	POINT ptCursor, ptCursorTransformed;

	GetCursorPos(&ptCursor);

	ptCursorTransformed = ptCursor;
	ScreenToClient(m_hTreeWindow, &ptCursorTransformed);

	std::wstring selected_item_path;
	m_hRightClickedItem = GetClickedTreeItemPath(m_hTreeWindow, ptCursorTransformed, selected_item_path);

	if (m_hRightClickedItem != nullptr)
	{
		HMENU hRClickMenu = CreateContextMenu(selected_item_path);

		TrackPopupMenu(hRClickMenu,
			TPM_TOPALIGN,
			ptCursor.x,
			ptCursor.y,
			NULL,
			m_hWndSelf,
			NULL
		);

		DestroyMenu(hRClickMenu);
	}
}

HMENU Explorer::CreateContextMenu(const std::wstring& path)
{
	HMENU hRClickMenu = CreatePopupMenu();

	bool isDirectory = Utility::IsPathDirectory(path);

	if (isDirectory) {
		
		HMENU hNewMenu = CreatePopupMenu();
		AppendMenu(hNewMenu, MF_STRING, IDC_CONTEXT_NEW_FILE, L"New file...\tCtrl+N");
		AppendMenu(hNewMenu, MF_STRING, IDC_CONTEXT_NEW_FOLDER, L"New folder...\tCtrl+Shift+N");
		AppendMenu(hRClickMenu, MF_POPUP, (UINT_PTR)hNewMenu, L"New");
	}

	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_OPEN, L"Open");
	AppendMenu(hRClickMenu, MF_SEPARATOR, NULL, nullptr);
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_OPEN_IN_FILE_EXPLORER, L"Open in File Explorer");
	AppendMenu(hRClickMenu, MF_SEPARATOR, NULL, nullptr);
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_RENAME, L"Rename");
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_COPY, L"Copy\tCtrl+C");
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_CUT, L"Cut\tCtrl+X");

	if (isDirectory)
	{
		UINT uFlags = MF_STRING;

		if (!m_Clipboard.CanPasteToDirectory())
		{
			uFlags |= MF_GRAYED;
		}

		AppendMenu(hRClickMenu, uFlags, IDC_CONTEXT_PASTE, L"Paste\tCtrl+V");
	}

	AppendMenu(hRClickMenu, MF_SEPARATOR, NULL, nullptr);
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_DELETE, L"Delete\tDel");

	return hRClickMenu;
}

LRESULT Explorer::OnCommand(HWND hWnd, WPARAM wParam)
{
	switch (wParam)
	{
	case IDC_CONTEXT_COPY:
		OnCopy();
		break;

	case IDC_CONTEXT_CUT:
		m_hItemToCut = m_hRightClickedItem;
		m_Clipboard.Cut(this);
		break;

	case IDC_CONTEXT_OPEN:
		OnContextOpen();
		break;

	case IDC_CONTEXT_OPEN_IN_FILE_EXPLORER:
		OnOpenInFileExplorer();
		break;

	case IDC_CONTEXT_RENAME:
		OnRename();
		break;

	case IDC_CONTEXT_PASTE:
		m_Clipboard.Paste(this);
		break;

	case IDC_CONTEXT_NEW_FILE:
		TreeView_SelectItem(m_hTreeWindow, m_hRightClickedItem);
		this->CreateNewFile();
		break;

	case IDC_CONTEXT_NEW_FOLDER:
		TreeView_SelectItem(m_hTreeWindow, m_hRightClickedItem);
		this->CreateNewFolder();
		break;

	case IDC_CONTEXT_DELETE:
	{
		int button_clicked = MessageBox(
			hWnd,
			L"Are you sure you want to permanently delete this item?",
			L"Delete item",
			MB_YESNO | MB_ICONEXCLAMATION
		);

		if (button_clicked == IDYES)
			OnDelete();
	}
		break;
	}

	return 0;
}

void Explorer::OnCopy(void)
{
	std::wstring path;
	this->GetItemPath(m_hTreeWindow, m_hRightClickedItem, path);
	m_Clipboard.Copy(path.c_str());
}

static TABINFO GetSourceTabFromAbsolutePath(LPCWSTR pAbsolutePath, WorkArea* pWorkArea)
{
	TABINFO tabInfo = {};

	TabList& visible_tabs = pWorkArea->GetVisibleTabs();

	for (size_t i = 0; i < visible_tabs.size(); ++i) 
	{
		if (lstrcmp(visible_tabs[i]->GetPath(), pAbsolutePath) == 0)
		{
			tabInfo.index = i;
			tabInfo.pSourceTab = visible_tabs[i];
			tabInfo.pTabList = &visible_tabs;
			return tabInfo;
		}
	}

	TabList& hidden_tabs = pWorkArea->GetHiddenTabs();

	for (size_t i = 0; i < hidden_tabs.size(); ++i)
	{
		if (lstrcmp(hidden_tabs[i]->GetPath(), pAbsolutePath) == 0)
		{
			tabInfo.index = i;
			tabInfo.pSourceTab = hidden_tabs[i];
			tabInfo.pTabList = &hidden_tabs;
			return tabInfo;
		}
	}

	return TABINFO();
}

static HRESULT SilentDeleteDirectory(const std::wstring& path)
{
	const std::wstring shPath = path + L'\0';

	SHFILEOPSTRUCT file_op = {
		NULL,
		FO_DELETE,
		shPath.c_str(),
		L"",
		FOF_NOCONFIRMATION |
		FOF_NOERRORUI |
		FOF_SILENT,
		false,
		0,
		L"" 
	};

	return SHFileOperation(&file_op) == 0 ? S_OK : E_FAIL;
}

static void DeleteTabFromWorkArea(TABINFO& tabInfo)
{
	InvalidateRect(GetParent(tabInfo.pSourceTab->GetHandle()), NULL, FALSE);

	tabInfo.pTabList->erase(tabInfo.pTabList->begin() + tabInfo.index);

	tabInfo.pSourceTab->Hide();
	DestroyWindow(tabInfo.pSourceTab->GetHandle());

	tabInfo.pSourceTab->GetSourceEdit()->Hide();
	DestroyWindow(tabInfo.pSourceTab->GetSourceEdit()->GetHandle());

	delete tabInfo.pSourceTab;
}

static void CloseDirectoryTabs(
	_In_ const std::wstring& directory,
	_In_ const HWND hTreeWindow,
	_In_ const HTREEITEM hFolderItem,
	_In_ WorkArea* pWorkArea
)
{
	HTREEITEM hItem = TreeView_GetChild(hTreeWindow, hFolderItem);

	while (hItem != nullptr)
	{
		wchar_t buffer[MAX_PATH];

		TVITEM tvItem;
		tvItem.cchTextMax = MAX_PATH;
		tvItem.pszText = buffer;
		tvItem.mask = TVIF_TEXT;
		tvItem.hItem = hItem;
		TreeView_GetItem(hTreeWindow, &tvItem);

		std::wstring file_path = (directory + L"\\") + buffer;

		if (!Utility::IsPathDirectory(file_path))
		{
			TABINFO tabInfo = GetSourceTabFromAbsolutePath(file_path.c_str(), pWorkArea);

			if (tabInfo.pSourceTab != nullptr)
			{
				DeleteTabFromWorkArea(tabInfo);
			}
		}

		else {
			CloseDirectoryTabs(file_path, hTreeWindow, hItem, pWorkArea);
		}

		hItem = TreeView_GetNextSibling(hTreeWindow, hItem);
	}
}

void Explorer::OnDelete(void)
{
	std::wstring path;
	this->GetItemPath(m_hTreeWindow, m_hRightClickedItem, path);

	m_pStatusBar->SetText(L"Deleting file...", 0);

	if (Utility::IsPathDirectory(path))
	{
		WorkArea* pWorkArea = GetAssociatedObject<AppWindow>(m_hWndParent)->GetWorkArea();

		CloseDirectoryTabs(
			path,
			m_hTreeWindow,
			m_hRightClickedItem,
			pWorkArea
		);

		pWorkArea->OnDPIChanged();
		InvalidateRect(pWorkArea->GetHandle(), NULL, FALSE);

		if (FAILED(SilentDeleteDirectory(path)))
		{
			m_pStatusBar->SetText(L"Folder deletion failed.", 0);
		}

		else
		{
			TreeView_DeleteItem(m_hTreeWindow, m_hRightClickedItem);
			m_hRightClickedItem = nullptr;
			m_pStatusBar->SetText(L"Folder deleted", 0);
		}
	}

	else if (DeleteFile(path.c_str()) != 0)
	{
		m_pStatusBar->SetText(L"File deleted", 0);

		TreeView_DeleteItem(m_hTreeWindow, m_hRightClickedItem);
		m_hRightClickedItem = nullptr;

		WorkArea* pWorkArea = GetAssociatedObject<AppWindow>(m_hWndParent)->GetWorkArea();

		TABINFO tInfo = GetSourceTabFromAbsolutePath(path.c_str(), pWorkArea);

		if (tInfo.pSourceTab)
		{
			DeleteTabFromWorkArea(tInfo);
			pWorkArea->OnDPIChanged();
		}
	}

	else {
		m_pStatusBar->SetText(L"File deletion failed.", 0);
	}
}

void Explorer::OnRename(void)
{
	// Undocumented for some reason but the return value
	// is the handle to the single-line edit control
	// that is created when the user starts editing
	m_hRenameEdit = TreeView_EditLabel(m_hTreeWindow, m_hRightClickedItem);
}

void Explorer::OnOpenInFileExplorer(void)
{
	std::wstring path;
	GetItemPath(m_hTreeWindow, m_hRightClickedItem, path);

	const wchar_t* p_Path = path.c_str();
	wchar_t cstr_path[MAX_PATH];

	if (!Utility::IsPathDirectory(path)) {
		lstrcpy(cstr_path, p_Path);
		PathRemoveFileSpec(cstr_path);
		p_Path = cstr_path;
	}

	ShellExecute(NULL, L"open", p_Path, NULL, NULL, SW_SHOWDEFAULT);

	m_pStatusBar->SetText(L"Opened folder in File Explorer", 0);
}

void Explorer::OnContextOpen(void)
{
	std::wstring selected;

	HTREEITEM hClicked = GetItemPath(m_hTreeWindow, m_hRightClickedItem, selected);

	if (hClicked != nullptr)
	{
		if (Utility::IsPathDirectory(selected.c_str()))
		{
			TreeView_Expand(m_hTreeWindow, hClicked, TVE_TOGGLE);
		}

		else
		{
			OpenTabFromFilePath(selected.c_str());
		}
	}
}

void Explorer::OpenTabFromFilePath(LPCWSTR lpFilePath)
{
	AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(m_hWndParent);

	if (pAppWindow != nullptr)
	{
		WorkArea* pWorkArea = pAppWindow->GetWorkArea();

		if (pWorkArea != nullptr)
		{
			pWorkArea->SelectFileFromName(const_cast<wchar_t*>(lpFilePath));
		}
	}
}

LRESULT Explorer::OnNCHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	POINT ptCursor = {};
	ptCursor.x = LOWORD(lParam);
	ptCursor.y = HIWORD(lParam);
	ScreenToClient(hWnd, &ptCursor);

	if (ptCursor.x >= m_rcSelf.right - HTRIGHT_WIDTH)
	{
		return HTRIGHT;
	}

	return DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);
}

LRESULT Explorer::OnGetMinMaxInfo(HWND hWnd, LPARAM lParam)
{
	LPMINMAXINFO pInfo = reinterpret_cast<LPMINMAXINFO>(lParam);

	pInfo->ptMinTrackSize.x = HTRIGHT_WIDTH;

	return 0;
}

LRESULT Explorer::OnSize(HWND hWnd, LPARAM lParam)
{
	m_rcSelf.right = LOWORD(lParam);
	m_rcSelf.bottom = HIWORD(lParam);

	AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(m_hWndParent);
	WorkArea* pWorkArea = pAppWindow->GetWorkArea();

	RECT rcClient;
	GetClientRect(GetAncestor(hWnd, GA_ROOT), &rcClient);

	if (pWorkArea)
	{
		pWorkArea->SetPos(m_rcSelf.right, 0);

		OutputContainer* pOutputContainer = pAppWindow->GetOutputWindow();

		if (pOutputContainer)
		{
			pOutputContainer->SetPos(
				m_rcSelf.right,
				pWorkArea->GetRect().bottom + 3
			);

			pOutputContainer->SetSize(
				rcClient.right - m_rcSelf.right,
				pOutputContainer->GetRect().bottom
			);
		}
	}

	m_rcTree.right = m_rcSelf.right - HTRIGHT_WIDTH;
	m_rcTree.bottom = m_rcSelf.bottom;

	SetWindowPos(m_hTreeWindow, nullptr, 0, 0, m_rcTree.right, m_rcTree.bottom, SWP_NOZORDER | SWP_NOMOVE);

	return 0;
}

#define BUFFER_SIZE (MAX_PATH + 1)

static inline bool IsHiddenFile(const wchar_t* lpszFileName)
{
	return lpszFileName[0] == L'.';
}

static inline bool IsDirectory(LPWIN32_FIND_DATA pFData)
{
	return pFData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

void Explorer::ExploreDirectory(const wchar_t* directory, HTREEITEM hParent)
{
	std::wstring wstr = directory;
	wstr.append(L"\\*");

	WIN32_FIND_DATA find_data;
	HANDLE hFind = FindFirstFile(wstr.c_str(), &find_data);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {
			if (!IsHiddenFile(find_data.cFileName))
			{
				if (IsDirectory(&find_data))
				{
					std::wstring new_dir = directory;
					new_dir += L'\\';
					new_dir += find_data.cFileName;
					HTREEITEM hItem = Utility::AddToTree(m_hTreeWindow, hParent, find_data.cFileName, true);
					ExploreDirectory(new_dir.c_str(), hItem);
				}

				else
				{
					Utility::AddToTree(m_hTreeWindow, hParent, find_data.cFileName, false);
				}
			}
		} while (FindNextFile(hFind, &find_data));

		FindClose(hFind);
	}

	else
	{
		Logger::Write(L"Folder \'%s\' was not found!", directory);
	}
}

static void WriteWindowTextToFile(HWND hWindow, std::wofstream& file)
{
	const size_t length = GetWindowTextLength(hWindow) + 1;

	wchar_t* buffer = new wchar_t[length];

	GetWindowText(hWindow, buffer, length);
	file.write(buffer, length);

	delete[] buffer;
}

void Explorer::SaveFileFromTab(SourceTab* pSourceTab)
{
	if (pSourceTab != nullptr)
	{
		SourceEdit* pSourceEdit = pSourceTab->GetSourceEdit();

		if (pSourceEdit != nullptr)
		{
			if (pSourceEdit->HasBeenEdited())
			{
				std::wofstream current_file(pSourceTab->GetPath(), std::ios::trunc | std::ios::binary);

				if (current_file.is_open())
				{
					WriteWindowTextToFile(pSourceEdit->GetHandle(), current_file);
					pSourceTab->RemoveAsteriskFromDisplayedName();
					pSourceEdit->MarkAsUnedited();
				}
			}
		}
	}
}

void Explorer::SaveCurrentFile(WorkArea* pWorkArea)
{
	if (pWorkArea != nullptr)
	{
		if (pWorkArea->GetSelectedTab()->GetSourceEdit()->HasBeenEdited())
		{
			SaveFileFromTab(pWorkArea->GetSelectedTab());
			m_pStatusBar->SetText(L"Saved file.", 0);
		}
	}
}

void Explorer::SaveAllFiles(WorkArea* pWorkArea)
{
	TabList& opened_tabs = pWorkArea->GetVisibleTabs();

	for (SourceTab* pSourceTab : opened_tabs)
	{
		SaveFileFromTab(pSourceTab);
	}

	TabList& hidden_tabs = pWorkArea->GetHiddenTabs();

	for (SourceTab* pSourceTab : hidden_tabs)
	{
		SaveFileFromTab(pSourceTab);
	}

	if (!opened_tabs.empty() || !hidden_tabs.empty())
	{
		m_pStatusBar->SetText(L"All files saved.", 0);
	}
}

HWND Explorer::GetTreeHandle(void) const
{
	return m_hTreeWindow;
}

void Explorer::OpenProjectFolder(std::wstring folder)
{
	TreeView_DeleteAllItems(m_hTreeWindow);

	const size_t last_backslash_index = folder.find_last_of(L'\\');

	m_RootDirectory = folder.substr(0, last_backslash_index + 1);

	HTREEITEM hRoot = Utility::SetItemAsTreeRoot(m_hTreeWindow, const_cast<wchar_t*>(folder.c_str() + last_backslash_index + 1));

	ExploreDirectory(folder.c_str(), hRoot);

	TreeView_Expand(m_hTreeWindow, hRoot, TVE_EXPAND | TVE_EXPANDPARTIAL);
}

enum class SIFD_CODE {
	SUCCESS,
	FAILED,
	FILE_ALREADY_EXISTS,
	ACCESS_DENIED,
	NAME_CONTAINS_INVALID_CHARS,
	NULL_DATA
};

enum class ItemType {
	FILE, FOLDER
};

struct EnterNameDialogData {
	const wchar_t* lpszTitle = nullptr;
	const wchar_t* lpszStatic = nullptr;
	wchar_t* lpszNewItemName = nullptr; // out
	ItemType type = ItemType::FILE;
	HTREEITEM hParentItem = nullptr;
	HWND hTree = nullptr;
	Explorer* pExplorer = nullptr;
};

static SIFD_CODE CreateItem(const std::wstring& absolute_path, ItemType type)
{
	if (type == ItemType::FOLDER) {
		CreateDirectory(absolute_path.c_str(), NULL);
		return SIFD_CODE::SUCCESS;
	}

	HANDLE hFile = CreateFile(
		absolute_path.c_str(),
		GENERIC_WRITE,
		NULL,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE) {
		return SIFD_CODE::FAILED;
	}

	CloseHandle(hFile);

	return SIFD_CODE::SUCCESS;
}

static SIFD_CODE SaveItemFromDialog(HWND hWnd, EnterNameDialogData* pData)
{
	if (pData != nullptr) 
	{
		wchar_t lpszBuffer[128];
		GetDlgItemText(hWnd, IDC_NAME_EDIT, lpszBuffer, 128);

		if (FileNameContainsInvalidCharacters(lpszBuffer)) 
		{
			return SIFD_CODE::NAME_CONTAINS_INVALID_CHARS;
		}

		std::wstring absolute_path;
		pData->pExplorer->GetItemPath(pData->hTree, pData->hParentItem, absolute_path);

		absolute_path = absolute_path + L"\\" + lpszBuffer;
			
		if (PathFileExists(absolute_path.c_str()))
		{
			return SIFD_CODE::FILE_ALREADY_EXISTS;
		}

		return CreateItem(absolute_path, pData->type);
	}

	return SIFD_CODE::NULL_DATA;
}

static void OnCommand(HWND hWnd, WPARAM wParam, EnterNameDialogData* pData)
{
	const wchar_t* lpszInvalidCharactersMsg = (pData->type == ItemType::FILE) ?
		L"File name contains invalid characters." :
		L"Folder name contains invalid characters.";

	const wchar_t* lpszInvalidCharacterTitle = (pData->type == ItemType::FILE) ?
		L"Unable to create file" :
		L"Unable to create folder";

	switch (wParam)
	{
	case IDOK:
		switch (SaveItemFromDialog(hWnd, pData))
		{
		case SIFD_CODE::SUCCESS:
			wchar_t lpszBuffer[128];
			GetDlgItemText(hWnd, IDC_NAME_EDIT, lpszBuffer, 128);
			pData->lpszNewItemName = _wcsdup(lpszBuffer);
			EndDialog(hWnd, IDOK);
			break;

		case SIFD_CODE::NAME_CONTAINS_INVALID_CHARS:
			MessageBox(
				hWnd,
				lpszInvalidCharactersMsg,
				lpszInvalidCharacterTitle,
				MB_OK | MB_ICONERROR
			);
			break;

		case SIFD_CODE::FILE_ALREADY_EXISTS:
			MessageBox(
				hWnd,
				L"File/Folder already exists.",
				lpszInvalidCharacterTitle,
				MB_OK | MB_ICONERROR
			);
			break;

		case SIFD_CODE::ACCESS_DENIED:
			MessageBox(
				hWnd,
				L"Access denied",
				lpszInvalidCharacterTitle,
				MB_OK | MB_ICONERROR
			);
			break;

		case SIFD_CODE::FAILED:
			MessageBox(
				hWnd,
				L"Creation failed!",
				lpszInvalidCharacterTitle,
				MB_OK | MB_ICONERROR
			);
			Logger::Write(L"%s creation failed! Error Code: %d", pData->type == ItemType::FILE ? "File" : "Folder", GetLastError());
			break;

		case SIFD_CODE::NULL_DATA:
			MessageBox(
				hWnd,
				L"Error",
				L"Data passed to EnterNameDialogProcedure is NULL",
				MB_OK | MB_ICONERROR
			);
			break;
		}
		break;

	case IDCLOSE:
		EndDialog(hWnd, IDCANCEL);
		break;
	}
}

static INT_PTR CALLBACK EnterNameDialogProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static EnterNameDialogData* pData = nullptr;

	switch (uMsg)
	{
	case WM_INITDIALOG: {
		pData = reinterpret_cast<EnterNameDialogData*>(lParam);
		SetWindowText(hWnd, pData->lpszTitle);
		SetDlgItemText(hWnd, IDC_MESSAGE_STATIC, pData->lpszStatic);
		SetFocus(GetDlgItem(hWnd, IDC_NAME_EDIT));
		return 0;
	}

	case WM_CLOSE:
		EndDialog(hWnd, IDCLOSE);
		return 0;

	case WM_COMMAND:
		OnCommand(hWnd, wParam, pData);
		return 0;
	}

	return 0;
}

void Explorer::CreateNewFile(void)
{
	HTREEITEM hItem = TreeView_GetSelection(m_hTreeWindow);

	if (hItem != nullptr) 
	{
		EnterNameDialogData data;
		data.lpszTitle = L"Create new file";
		data.lpszStatic = L"Enter the name of the new file:";
		data.hParentItem = hItem;
		data.type = ItemType::FILE;
		data.pExplorer = this;
		data.hTree = m_hTreeWindow;

		INT_PTR status = DialogBoxParam(
			GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_ENTER_NAME_DIALOG),
			m_hWndSelf,
			EnterNameDialogProcedure,
			(LPARAM)&data
		);

		if (status == IDOK) {
			Utility::AddToTree(m_hTreeWindow, hItem, data.lpszNewItemName, false);
			UpdateWindow(m_hTreeWindow);
			free(data.lpszNewItemName);
		}
	}
}

void Explorer::CreateNewFolder(void)
{
	HTREEITEM hItem = TreeView_GetSelection(m_hTreeWindow);

	if (hItem != nullptr)
	{
		EnterNameDialogData data;
		data.lpszTitle = L"Create new folder";
		data.lpszStatic = L"Enter the name of the new folder:";
		data.hParentItem = hItem;
		data.type = ItemType::FOLDER;
		data.pExplorer = this;
		data.hTree = m_hTreeWindow;

		INT_PTR status = DialogBoxParam(
			GetModuleHandle(NULL),
			MAKEINTRESOURCE(IDD_ENTER_NAME_DIALOG),
			m_hWndSelf,
			EnterNameDialogProcedure,
			(LPARAM)&data
		);

		if (status == IDOK) {
			Utility::AddToTree(m_hTreeWindow, hItem, data.lpszNewItemName, true);
			free(data.lpszNewItemName);
		}
	}
}

static LRESULT OnCreate(HWND hWnd, LPARAM lParam)
{
	SetWindowLongPtr(
		hWnd, GWLP_USERDATA,
		reinterpret_cast<LONG>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams)
	);

	return 0;
}

static LRESULT CALLBACK ExplorerWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return OnCreate(hWnd, lParam);

	default:
		Explorer* pExplorer = GetAssociatedObject<Explorer>(hWnd);

		if (pExplorer != nullptr)
		{
			return pExplorer->WindowProcedure(hWnd, uMsg, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}