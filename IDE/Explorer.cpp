#include "Explorer.h"
#include "WorkArea.h"
#include "Logger.h"
#include "Utility.h"
#include "AppWindow.h"

#define EXPLORER_WINDOW_CLASS L"IDEExplorerWindowClass"

static HRESULT RegisterExplorerWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK ExplorerWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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
		wcex.style = CS_HREDRAW | CS_VREDRAW;

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

	if (pWorkArea)
	{
		RECT rcClient;
		GetClientRect(GetAncestor(hWnd, GA_ROOT), &rcClient);

		pWorkArea->SetPos(m_rcSelf.right + 3, 0);
		pWorkArea->SetSize(rcClient.right - m_rcSelf.right - 6, m_rcSelf.bottom);
	}

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