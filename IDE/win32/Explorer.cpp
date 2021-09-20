#include "Explorer.h"
#include "WorkArea.h"
#include "Logger.h"
#include "Utility.h"
#include "AppWindow.h"

#include <CommCtrl.h>
#include <windowsx.h>

#define EXPLORER_WINDOW_CLASS L"IDEExplorerWindowClass"

static HRESULT RegisterExplorerWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK ExplorerWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

HTREEITEM AddItemToTree(HWND hwndTV, LPTSTR lpszItem, int nLevel)
{
	TVITEM tvi;
	TVINSERTSTRUCT tvins;
	static HTREEITEM hPrev = (HTREEITEM)TVI_FIRST;
	static HTREEITEM hPrevRootItem = NULL;
	static HTREEITEM hPrevLev2Item = NULL;

	tvi.mask = TVIF_TEXT | TVIF_PARAM;

	// Set the text of the item. 
	tvi.pszText = lpszItem;
	tvi.cchTextMax = lstrlen(lpszItem);

	// Save the heading level in the item's application-defined 
	// data area. 
	tvi.lParam = (LPARAM)nLevel;
	tvins.item = tvi;
	tvins.hInsertAfter = hPrev;

	// Set the parent item based on the specified level. 
	if (nLevel == 1)
		tvins.hParent = TVI_ROOT;
	else if (nLevel == 2)
		tvins.hParent = hPrevRootItem;
	else
		tvins.hParent = hPrevLev2Item;

	// Add the item to the tree-view control. 
	hPrev = (HTREEITEM)SendMessage(hwndTV, TVM_INSERTITEM,
		0, (LPARAM)(LPTVINSERTSTRUCT)&tvins);

	if (hPrev == NULL)
		return NULL;

	// Save the handle to the item. 
	if (nLevel == 1)
		hPrevRootItem = hPrev;
	else if (nLevel == 2)
		hPrevLev2Item = hPrev;

	return hPrev;
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
			WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_LINESATROOT | 
			TVS_HASBUTTONS | WS_CLIPCHILDREN | TVS_EDITLABELS,
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
		}
	}

	return m_hWndSelf ? S_OK : E_FAIL;
}

static void GetTreeItemPath(
	_In_  HWND hTreeWindow,
	_Out_ std::wstring& path
)
{
	path.clear();

	wchar_t buf[128];
	wchar_t file_name[128];

	TVITEM item;
	item.mask = TVIF_TEXT;
	item.hItem = TreeView_GetSelection(hTreeWindow);
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

	if (!path.empty())
	{
		path.append(file_name);
	}
}

LRESULT Explorer::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_NCHITTEST:
		return OnNCHitTest(hWnd, wParam, lParam);

	case WM_GETMINMAXINFO:
		return OnGetMinMaxInfo(hWnd, lParam);

	case WM_SIZE:
		return OnSize(hWnd, lParam);

	case WM_NOTIFY:
		return OnNotify(hWnd, lParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
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
		std::wstring selection_path;
		::GetTreeItemPath(m_hTreeWindow, selection_path);

		if (!selection_path.empty() && !IsPathDirectory(selection_path))
		{
			GetAssociatedObject<AppWindow>(m_hWndParent)
				->GetWorkArea()
				->SelectFileFromName(
					const_cast<wchar_t*>(selection_path.c_str())
				);
		}
	}

	return 0;
}

LRESULT Explorer::OnNCHitTest(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	POINT ptCursor;
	ptCursor.x = LOWORD(lParam);
	ptCursor.y = HIWORD(lParam);
	ScreenToClient(hWnd, &ptCursor);

	if (ptCursor.x >= m_rcSelf.right - 10)
	{
		return HTRIGHT;
	}

	return DefWindowProc(hWnd, WM_NCHITTEST, wParam, lParam);
}

LRESULT Explorer::OnGetMinMaxInfo(HWND hWnd, LPARAM lParam)
{
	LPMINMAXINFO pInfo = reinterpret_cast<LPMINMAXINFO>(lParam);

	pInfo->ptMinTrackSize.x = static_cast<int>(200 * Utility::GetScaleForDPI(m_hWndParent));
	pInfo->ptMaxTrackSize.x = static_cast<int>(400 * Utility::GetScaleForDPI(m_hWndParent));

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

int depth = 1;

static inline bool IsHiddenFile(const wchar_t* lpszFileName)
{
	return lpszFileName[0] == L'.';
}

static inline bool IsDirectory(LPWIN32_FIND_DATA pFData)
{
	return pFData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
}

void Explorer::ExploreDirectory(const wchar_t* directory)
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
					AddItemToTree(m_hTreeWindow, find_data.cFileName, depth);
					++depth;
					ExploreDirectory(new_dir.c_str());
				}

				else
				{
					AddItemToTree(m_hTreeWindow, find_data.cFileName, depth);
				}
			}
		} while (FindNextFile(hFind, &find_data));

		FindClose(hFind);

		--depth;
	}

	else
	{
		Logger::Write(L"Folder \'%s\' was not found!\n", directory);
	}
}

void Explorer::OpenProjectFolder(std::wstring folder)
{
	int pointer_offset = 0;

	if (folder[0] == L'\"')
	{
		pointer_offset = 1;
		folder.back() = L'\0';
	}

	TreeView_DeleteAllItems(m_hTreeWindow);

	AddItemToTree(m_hTreeWindow, (wchar_t*)folder.c_str() + pointer_offset, 1);

	depth = 2;

	ExploreDirectory(folder.c_str() + pointer_offset);
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