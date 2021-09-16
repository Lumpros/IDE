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
	SAFE_DELETE_PTR(m_pOutput);
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

	RECT rcClient;
	GetClientRect(m_hWndSelf, &rcClient);
	m_rcSelf.right = rcClient.right;
	m_rcSelf.bottom = rcClient.bottom;

	if (FAILED(InitializeComponents()))
	{
		Logger::Write(L"Failed to initialize one or more components");
		return E_FAIL;
	}

	Utility::CenterWindowRelativeToParent(m_hWndSelf);

	ShowWindow(m_hWndSelf, SW_SHOWMAXIMIZED);
	UpdateWindow(m_hWndSelf);

	return S_OK;
}

HRESULT AppWindow::InitializeComponents(void)
{
	HRESULT hr = E_FAIL;

	m_pWorkArea = new WorkArea(m_hWndSelf);

	if (m_pWorkArea)
	{
		m_pExplorer = new Explorer(m_hWndSelf);
		
		if (m_pExplorer)
		{
			m_pExplorer->SetSize(static_cast<int>(250 * Utility::GetScaleForDPI(m_hWndSelf)), 0);
			m_pWorkArea->SetSize(m_rcSelf.right - m_pExplorer->GetRect().right - 6, m_rcSelf.bottom * 2 / 3);

			m_pOutput = new Output(m_hWndSelf);

			if (m_pOutput)
			{
				m_pOutput->SetPos(m_pExplorer->GetRect().right + 3, m_rcSelf.bottom * 3 / 4 + 3);
				m_pOutput->SetSize(m_rcSelf.right - m_pExplorer->GetRect().right - 6, m_rcSelf.bottom / 4 - 3);

				hr = S_OK;
			}
		}
	}

	return hr;
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

	if (m_pExplorer)
	{
		const int iExplorerWidth = m_pExplorer->GetRect().right;
		const int iWorkAreaHeight = m_pWorkArea->GetRect().bottom;

		m_pExplorer->SetSize(iExplorerWidth, m_rcSelf.bottom);

		if (m_pWorkArea)
		{
			m_pWorkArea->SetPos(iExplorerWidth + 3, 0);

			if (m_pOutput)
			{
				m_pOutput->SetPos(iExplorerWidth + 3, m_rcSelf.bottom - m_pOutput->GetRect().bottom);
				m_pOutput->SetSize(m_rcSelf.right - iExplorerWidth - 3, m_pOutput->GetRect().bottom);
			}
		}
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
		ShowWindow(hWnd, SW_HIDE);
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