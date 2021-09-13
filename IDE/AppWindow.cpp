#include "AppWindow.h"
#include "Utility.h"
#include "Logger.h"
#include "resource.h"
#include "ColorFormatParser.h"

#define APP_WINDOW_CLASS L"IDEAppWindow"

static HRESULT RegisterAppWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK AppWindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

AppWindow::~AppWindow(void)
{
	SAFE_DELETE_PTR(m_pExplorer);
	SAFE_DELETE_PTR(m_pWorkArea);
}

void AppWindow::Initialize(HINSTANCE hInstance)
{
	if (FAILED(::RegisterAppWindowClass(hInstance)))
	{
		Logger::Write(L"Failed to register app window class!");
		Logger::CloseOutputFile();
		ExitProcess(1);
	}

	if (FAILED(InitializeWindow(hInstance)))
	{
		Logger::Write(L"Window initialization failed!");
		Logger::CloseOutputFile();
		ExitProcess(1);
	}
}

static HRESULT RegisterAppWindowClass(HINSTANCE hInstance)
{
	static bool hasBeenRegistered = false;

	if (!hasBeenRegistered)
	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(WNDCLASSEX));
		wcex.cbSize = sizeof(wcex);
		wcex.lpszClassName = APP_WINDOW_CLASS;
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.lpfnWndProc = ::AppWindowProcedure;
		wcex.cbWndExtra = sizeof(AppWindow*);
		wcex.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcex))
			return E_FAIL;
		
		hasBeenRegistered = true;
	}

	return S_OK;
}

HRESULT AppWindow::InitializeWindow(HINSTANCE hInstance)
{
	m_hWndSelf = CreateWindowEx(
		NULL,
		APP_WINDOW_CLASS,
		L"IDE",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1280, 960,
		nullptr,
		nullptr,
		hInstance,
		this
	);

	if (m_hWndSelf == nullptr)
	{
		return E_FAIL;
	}

	SetMenu(m_hWndSelf,
		LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1))
	);

	m_pWorkArea = new WorkArea(m_hWndSelf);

	m_pExplorer = new Explorer(m_hWndSelf);
	m_pExplorer->SetSize((int)(250 * Utility::GetScaleForDPI(m_hWndSelf)), 0);

	Utility::CenterWindowRelativeToParent(m_hWndSelf);

	ShowWindow(m_hWndSelf, SW_SHOWMAXIMIZED);
	UpdateWindow(m_hWndSelf);

	return S_OK;
}

LRESULT AppWindow::WindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_SIZE:
		return OnSize(hWnd, lParam);

	case WM_GETMINMAXINFO:
		return OnGetMinMax(hWnd, lParam);

	case WM_DPICHANGED:
		return OnDPIChanged(hWnd, lParam);
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

LRESULT AppWindow::OnSize(HWND hWnd, LPARAM lParam)
{
	m_rcSelf.right = LOWORD(lParam);
	m_rcSelf.bottom = HIWORD(lParam);

	int iExplorerWidth = 0;

	if (m_pExplorer)
	{
		iExplorerWidth = m_pExplorer->GetRect().right;
		m_pExplorer->SetSize(iExplorerWidth, m_rcSelf.bottom);
	}

	if (m_pWorkArea)
	{
		m_pWorkArea->SetPos(iExplorerWidth + 3, 0);
		m_pWorkArea->SetSize(
			m_rcSelf.right - iExplorerWidth - 6,
			m_rcSelf.bottom
		);
	}

	return 0;
}
 
LRESULT AppWindow::OnGetMinMax(HWND hWnd, LPARAM lParam)
{
	return 0;
}

LRESULT AppWindow::OnDPIChanged(HWND hWnd, LPARAM lParam)
{
	LPRECT pRect = reinterpret_cast<LPRECT>(lParam);

	Utility::UpdateFont(hWnd);

	SetWindowPos(hWnd,
		nullptr,
		pRect->left,
		pRect->top,
		pRect->right,
		pRect->bottom, 
		SWP_NOZORDER
	);

	m_pWorkArea->OnDPIChanged();

	return 0;
}

static LRESULT OnCreate(HWND hWnd, LPARAM lParam)
{
	LPCREATESTRUCT lpParams = reinterpret_cast<LPCREATESTRUCT>(lParam);
	
	SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG>(lpParams->lpCreateParams));

	Utility::UpdateFont(hWnd);

	return 0;
}

static LRESULT CALLBACK AppWindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_CREATE:
		return OnCreate(hWnd, lParam);

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	case WM_QUIT:
		Logger::CloseOutputFile();
		return 0;

	default:
		AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(hWnd);

		if (pAppWindow != nullptr)
		{
			return pAppWindow->WindowProcedure(hWnd, uMessage, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}