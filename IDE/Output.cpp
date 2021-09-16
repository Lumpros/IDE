#include "Output.h"
#include "Logger.h"
#include "AppWindow.h"
#include "Utility.h"

#define OUTPUT_CLASS L"IDEOutputWindowClass"

#define IDC_OUTPUT_BUTTON 200
#define IDC_ERROR_LIST_BUTTON 201

static HRESULT RegisterOutputWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK OutputWindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

constexpr COLORREF crButton = RGB(3, 161, 252);

Output::Output(HWND hWnd)
{
	m_hWndParent = hWnd;
	
	const HINSTANCE hInstance = GetModuleHandle(NULL);

	if (FAILED(RegisterOutputWindowClass(hInstance)))
	{
		Logger::Write(L"Failed to register output window class!");
		PostQuitMessage(1);
		return;
	}

	if (FAILED(InitializeOutputWindow(hInstance)))
	{
		Logger::Write(L"Failed to create the output window!");
		PostQuitMessage(1);
		return;
	}
}

Output::~Output(void)
{
	SAFE_DELETE_PTR(m_pErrorListButton);
	SAFE_DELETE_PTR(m_pOutputButton);
}

static HRESULT RegisterOutputWindowClass(HINSTANCE hInstance)
{
	static bool hasBeenRegistered = false;

	if (!hasBeenRegistered)
	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(WNDCLASSEX));
		wcex.cbSize = sizeof(wcex);
		wcex.lpszClassName = OUTPUT_CLASS;
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpfnWndProc = ::OutputWindowProcedure;
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.cbWndExtra = sizeof(Output*);

		if (!RegisterClassEx(&wcex))
			return E_FAIL;

		hasBeenRegistered = true;
	}

	return S_OK;
}

static void DrawOutputButton(LPDRAWITEMSTRUCT lpDIS, LPWSTR lpszText, BOOLEAN isHovering, BOOLEAN isClicking)
{
	SelectObject(lpDIS->hDC, GetStockObject(DC_BRUSH));
	SelectObject(lpDIS->hDC, GetStockObject(DC_PEN));

	COLORREF cr = ::crButton;

	if (isHovering)
	{
		cr = RGB(23, 181, 255);

		if (isClicking)
		{
			cr = RGB(0, 150, 230);
		}
	}

	SetDCBrushColor(lpDIS->hDC, cr);
	SetDCPenColor(lpDIS->hDC, cr);
	Rectangle(lpDIS->hDC, 0, 0, lpDIS->rcItem.right, lpDIS->rcItem.bottom);

	const int length = lstrlen(lpszText);

	SetBkMode(lpDIS->hDC, TRANSPARENT);
	SetTextColor(lpDIS->hDC, 0xFFFFFF);
	SelectObject(lpDIS->hDC, Utility::GetStandardFont());

	SIZE size;
	GetTextExtentPoint32(lpDIS->hDC, lpszText, length, &size);

	TextOut(
		lpDIS->hDC,
		(lpDIS->rcItem.right - size.cx) / 2,
		(lpDIS->rcItem.bottom - size.cy) / 2,
		lpszText,
		length
	);
}

HRESULT Output::InitializeOutputWindow(HINSTANCE hInstance)
{
	m_hWndSelf = CreateWindow(
		OUTPUT_CLASS,
		nullptr,
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		m_hWndParent,
		nullptr,
		hInstance,
		this
	);

	if (!m_hWndSelf)
		return E_FAIL;

	BUTTONINFO bInfo;
	bInfo.rect = { 0, 0, 0, 0 };
	bInfo.lpszText = L"Output";
	bInfo.lpfnDrawFunc = ::DrawOutputButton;
	m_pOutputButton = new ODButton(m_hWndSelf, bInfo, IDC_OUTPUT_BUTTON);

	bInfo.lpszText = L"Error List";
	m_pErrorListButton = new ODButton(m_hWndSelf, bInfo, IDC_ERROR_LIST_BUTTON);

	return S_OK;
}

LRESULT Output::WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		return OnSize(hWnd, lParam);

	case WM_NCHITTEST:
		return OnNCHitTest(hWnd);

	case WM_WINDOWPOSCHANGED:
		return OnWindowPosChanged(hWnd, lParam);

	case WM_ERASEBKGND:
		return FALSE;

	case WM_PAINT:
		return OnPaint(hWnd);

	case WM_GETMINMAXINFO:
		return OnGetMinMaxInfo(hWnd, lParam);

	case WM_DRAWITEM:
		if (m_pErrorListButton && m_pOutputButton)
		{
			LPDRAWITEMSTRUCT lpDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
			m_pErrorListButton->Draw(lpDIS);
			m_pOutputButton->Draw(lpDIS);
		}
		return 0;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT Output::OnGetMinMaxInfo(HWND hWnd, LPARAM lParam)
{
	LPMINMAXINFO mmi = reinterpret_cast<LPMINMAXINFO>(lParam);

	mmi->ptMinTrackSize.y = this->GetMinimumHeight();

	return 0;
}

LRESULT Output::OnSize(HWND hWnd, LPARAM lParam)
{
	m_rcSelf.right = LOWORD(lParam);
	m_rcSelf.bottom = HIWORD(lParam);

	return 0;
}

LRESULT Output::OnNCHitTest(HWND hWnd)
{
	POINT ptCursor;
	GetCursorPos(&ptCursor);

	RECT rcWindow;
	GetWindowRect(hWnd, &rcWindow);

	if (ptCursor.y <= rcWindow.top + 10)
		return HTTOP;

	return HTCLIENT;
}

LRESULT Output::OnWindowPosChanged(HWND hWnd, LPARAM lParam)
{
	WINDOWPOS* pWindowPos = reinterpret_cast<WINDOWPOS*>(lParam);
	AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(m_hWndParent);

	if (pAppWindow)
	{
		WorkArea* pWorkArea = pAppWindow->GetWorkArea();
		Explorer* pExplorer = pAppWindow->GetExplorer();

		m_rcSelf.right = pWindowPos->cx;
		m_rcSelf.bottom = pWindowPos->cy;

		RECT rcParent;
		GetClientRect(m_hWndParent, &rcParent);

		if (pWorkArea && pExplorer)
		{
			pWorkArea->SetSize(
				rcParent.right - pExplorer->GetRect().right - 3,
				rcParent.bottom - pWindowPos->cy - 3
			);
		}

		const int min_height = GetMinimumHeight();

		if (m_pOutputButton && m_pErrorListButton)
		{
			m_pOutputButton->SetRect(RECT{ 0, 0, 100, min_height });
			m_pErrorListButton->SetRect(RECT{ 100, 0, 100, min_height });
		}
	}

	return 0;
}

LRESULT Output::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SelectObject(hDC, GetStockObject(DC_PEN));

	SetDCBrushColor(hDC, crButton);
	SetDCPenColor(hDC, crButton);

	const int rect_height = GetMinimumHeight();

	Rectangle(hDC, 0, 0, m_rcSelf.right, rect_height);

	SelectObject(hDC, Utility::GetStandardFont());
	SetBkMode(hDC, TRANSPARENT);
	SetTextColor(hDC, 0xFFFFFF);

	TEXTMETRIC tm;
	GetTextMetrics(hDC, &tm);

	TextOut(hDC, 5, (rect_height -  tm.tmHeight) / 2, L"Output", 6);

	RECT rc = { 0, rect_height, m_rcSelf.right, m_rcSelf.bottom };
	FillRect(hDC, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));

	EndPaint(hWnd, &ps);
	return 0;
}

int Output::GetMinimumHeight(void)
{
	return static_cast<const int>(20 * Utility::GetScaleForDPI(m_hWndParent));
}

static LRESULT OnCreate(HWND hWnd, LPARAM lParam)
{
	SetWindowLongPtr(
		hWnd, GWLP_USERDATA,
		reinterpret_cast<LONG>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams)
	);

	return 0;
}

static LRESULT CALLBACK OutputWindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_CREATE:
		return OnCreate(hWnd, lParam);

	default:
		Output* pOutput = GetAssociatedObject<Output>(hWnd);

		if (pOutput)
		{
			return pOutput->WindowProcedure(hWnd, uMessage, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}