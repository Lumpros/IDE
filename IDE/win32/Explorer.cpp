#include "Explorer.h"
#include "WorkArea.h"
#include "Logger.h"
#include "Utility.h"
#include "AppWindow.h"

#include <shellapi.h>
#include <CommCtrl.h>
#include <windowsx.h>

#define EXPLORER_WINDOW_CLASS L"IDEExplorerWindowClass"

#define IDC_CONTEXT_OPEN 3000
#define IDC_CONTEXT_RENAME 3001
#define IDC_CONTEXT_COPY 3002
#define IDC_CONTEXT_CUT 3003
#define IDC_CONTEXT_DELETE 3004

static HRESULT RegisterExplorerWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK ExplorerWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static HTREEITEM AddToTree(HWND hTreeView, HTREEITEM hParent, LPWSTR lpszItem, bool isDirectory)
{
	TVITEM tvItem;
	tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvItem.pszText = lpszItem;
	tvItem.cchTextMax = lstrlen(lpszItem);
	tvItem.iImage = isDirectory ? 0 : 1;
	tvItem.iSelectedImage = isDirectory ? 0 : 1;
	
	TVINSERTSTRUCT tvInsert;
	tvInsert.item = tvItem;
	tvInsert.hInsertAfter = TreeView_GetPrevSibling(hTreeView, TreeView_GetChild(hTreeView, hParent));
	tvInsert.hParent = hParent;

	return TreeView_InsertItem(hTreeView, &tvInsert);
}

static HTREEITEM SetItemAsTreeRoot(HWND hTreeView, LPWSTR lpszItem)
{
	TVITEM tvItem;
	tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
	tvItem.cchTextMax = lstrlen(lpszItem);
	tvItem.pszText = lpszItem;
	tvItem.iImage = 0;
	tvItem.iSelectedImage = 0;

	TVINSERTSTRUCT tvInsert;
	tvInsert.hParent = TVI_ROOT;
	tvInsert.hInsertAfter = TVI_FIRST;
	tvInsert.item = tvItem;

	return TreeView_InsertItem(hTreeView, &tvInsert);
}

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
			WS_VISIBLE | WS_CHILD | 
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

		hImageList = ImageList_Create(20, 20, ILC_COLOR32, 2, 0);


		// Get the folder icon
		SHFILEINFO sfi;
		SecureZeroMemory(&sfi, sizeof(sfi));
		SHGetFileInfo(L"something", FILE_ATTRIBUTE_DIRECTORY, &sfi, sizeof(sfi), SHGFI_ICON | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
		ImageList_AddIcon(hImageList, sfi.hIcon);

		// Get the document icon
		SHSTOCKICONINFO sInfo;
		sInfo.cbSize = sizeof(sInfo);
		SHGetStockIconInfo(SIID_DOCNOASSOC, SHGSI_ICON, &sInfo);

		ImageList_AddIcon(hImageList, sInfo.hIcon);

		TreeView_SetImageList(m_hTreeWindow, hImageList, TVSIL_NORMAL);
	}

	return m_hWndSelf ? S_OK : E_FAIL;
}

Explorer::~Explorer(void)
{
	ImageList_Destroy(hImageList);

	SAFE_DELETE_GDIOBJ(hFileIcon);
}

void Explorer::CloseProjectFolder(void)
{
	TreeView_DeleteAllItems(m_hTreeWindow);
}

static HTREEITEM GetClickedTreeItemPath(
	_In_  HWND hTreeWindow,
	_In_  POINT ptClick,
	_Out_ std::wstring& path
)
{
	path.clear();

	wchar_t buf[128];
	wchar_t file_name[128];

	TVHITTESTINFO htInfo;
	htInfo.pt = ptClick;

	TVITEM item;
	item.mask = TVIF_TEXT;
	item.hItem = TreeView_HitTest(hTreeWindow, &htInfo);
	item.cchTextMax = 128;
	item.pszText = file_name;
	TreeView_GetItem(hTreeWindow, &item);

	const HTREEITEM hReturn = item.hItem;

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

	path.append(file_name);

	return hReturn;
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
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Explorer::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

	SelectObject(hDC, GetStockObject(WHITE_BRUSH));
	SelectObject(hDC, GetStockObject(WHITE_PEN));

	Rectangle(hDC, m_rcSelf.right - 10, 0, m_rcSelf.right, m_rcSelf.bottom);

	EndPaint(hWnd, &ps);
	return 0;
}

static bool IsPathDirectory(const std::wstring& path)
{
	struct _stat64i32 s;
	_wstat64i32(path.c_str(), &s);

	return s.st_mode & S_IFDIR;
}

LRESULT Explorer::OnNotify(HWND hWnd, LPARAM lParam)
{
	LPNMHDR info = reinterpret_cast<LPNMHDR>(lParam);

	if (info->code == NM_DBLCLK)
	{
		POINT ptCursor;
		GetCursorPos(&ptCursor);
		ScreenToClient(m_hTreeWindow, &ptCursor);

		std::wstring selection_path;
		::GetClickedTreeItemPath(m_hTreeWindow, ptCursor, selection_path);

		if (!selection_path.empty() && !IsPathDirectory(selection_path))
		{
			OpenTabFromFilePath(selection_path.c_str());
		}
	}

	else if (info->code == NM_RCLICK)
	{
		OnRClickCreateContextMenu();
	}

	return 0;
}

void Explorer::OnRClickCreateContextMenu(void)
{
	HMENU hRClickMenu = CreatePopupMenu();

	//std::wstring selected_item_path;
	//GetTreeItemPath(m_hTreeWindow, selected_item_path);
	
	GetCursorPos(&m_ptCursorOnRClick);

	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_OPEN, L"Open");
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_RENAME, L"Rename");
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_COPY, L"Copy\tCtrl+C");
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_CUT, L"Cut\tCtrl+X");
	AppendMenu(hRClickMenu, MF_SEPARATOR, NULL, nullptr);
	AppendMenu(hRClickMenu, MF_STRING, IDC_CONTEXT_DELETE, L"Delete\tDel");

	TrackPopupMenu(hRClickMenu,
		TPM_TOPALIGN,
		m_ptCursorOnRClick.x,
		m_ptCursorOnRClick.y,
		NULL,
		m_hWndSelf,
		NULL
	);

	ScreenToClient(m_hTreeWindow, &m_ptCursorOnRClick);

	DestroyMenu(hRClickMenu);
}

LRESULT Explorer::OnCommand(HWND hWnd, WPARAM wParam)
{
	switch (wParam)
	{
	case IDC_CONTEXT_OPEN:
		OnContextOpen();
		break;
	}

	return 0;
}

void Explorer::OnContextOpen(void)
{
	std::wstring selected;

	HTREEITEM hClicked = ::GetClickedTreeItemPath(m_hTreeWindow, m_ptCursorOnRClick, selected);

	if (IsPathDirectory(selected.c_str()))
	{
		TreeView_Expand(m_hTreeWindow, hClicked, TVE_EXPAND | TVE_EXPANDPARTIAL);
	}

	else
	{
		OpenTabFromFilePath(selected.c_str());
	}
}

void Explorer::OpenTabFromFilePath(LPCWSTR lpFilePath)
{
	GetAssociatedObject<AppWindow>(m_hWndParent)
		->GetWorkArea()
		->SelectFileFromName(
			const_cast<wchar_t*>(lpFilePath)
		);
}

LRESULT Explorer::OnNCHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	POINT ptCursor;
	ptCursor.x = LOWORD(lParam);
	ptCursor.y = HIWORD(lParam);
	ScreenToClient(hWnd, &ptCursor);

	if (ptCursor.x >= m_rcSelf.right - 9)
	{
		return HTRIGHT;
	}

	return DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);
}

LRESULT Explorer::OnGetMinMaxInfo(HWND hWnd, LPARAM lParam)
{
	LPMINMAXINFO pInfo = reinterpret_cast<LPMINMAXINFO>(lParam);

	pInfo->ptMinTrackSize.x = 9;

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
		pWorkArea->SetPos(m_rcSelf.right + 3, 0);

		OutputContainer* pOutputContainer = pAppWindow->GetOutputWindow();

		if (pOutputContainer)
		{
			pOutputContainer->SetPos(m_rcSelf.right + 3, pWorkArea->GetRect().bottom + 3);
			pOutputContainer->SetSize(rcClient.right - m_rcSelf.right - 3, pOutputContainer->GetRect().bottom);
		}
	}

	m_rcTree.right = m_rcSelf.right - 10;
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
					HTREEITEM hItem = AddToTree(m_hTreeWindow, hParent, find_data.cFileName, true);
					ExploreDirectory(new_dir.c_str(), hItem);
				}

				else
				{
					AddToTree(m_hTreeWindow, hParent, find_data.cFileName, false);
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

void Explorer::OpenProjectFolder(std::wstring folder)
{
	int pointer_offset = 0;

	if (folder[0] == L'\"')
	{
		/* instead of manipulating the string */
		/* simply just pass the string starting from the second character */
		/* in order to skip the " character */
		pointer_offset = 1;

		folder.back() = L'\0';
	}

	TreeView_DeleteAllItems(m_hTreeWindow);

	HTREEITEM hRoot = SetItemAsTreeRoot(m_hTreeWindow, const_cast<wchar_t*>(folder.c_str()) + pointer_offset);

	ExploreDirectory(folder.c_str() + pointer_offset, hRoot);

	TreeView_Expand(m_hTreeWindow, hRoot, TVE_EXPAND | TVE_EXPANDPARTIAL);
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