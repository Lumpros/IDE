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

		AddItemToTree(m_hTreeWindow, (wchar_t*)L"Project", 1);
		AddItemToTree(m_hTreeWindow, (wchar_t*)L"Header Files", 2);
		AddItemToTree(m_hTreeWindow, (wchar_t*)L"app.h", 3);
		AddItemToTree(m_hTreeWindow, (wchar_t*)L"Source Files", 2);
		AddItemToTree(m_hTreeWindow, (wchar_t*)L"main.c", 3);
		AddItemToTree(m_hTreeWindow, (wchar_t*)L"app.c", 3);
	}

	return m_hWndSelf ? S_OK : E_FAIL;
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
	{
		LPNMHDR info = reinterpret_cast<LPNMHDR>(lParam);
		
		if (info->code == NM_DBLCLK)
		{
			HTREEITEM hTreeItem = TreeView_GetSelection(m_hTreeWindow);
			wchar_t buf[128];

			TVITEM item;
			item.hItem = hTreeItem;
			item.mask = TVIF_TEXT | TVIF_CHILDREN;
			item.cchTextMax = 128;
			item.pszText = buf;
			TreeView_GetItem(m_hTreeWindow, &item);

			if (item.cChildren == 0)
			{
				GetAssociatedObject<AppWindow>(m_hWndParent)->GetWorkArea()->SelectFileFromName(item.pszText);
			}
		}
	}
	return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
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