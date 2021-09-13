#include "SourceTab.h"
#include "Logger.h"
#include "Utility.h"
#include "WorkArea.h"

#define SOURCE_TAB_CLASS L"IDESourceTabClass"

#define IDC_CLOSE_BUTTON 100

static HRESULT RegisterSourceTabWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK SourceTabWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

SourceTab::~SourceTab(void)
{
	SAFE_DELETE_PTR(m_sInfo.m_pSourceEdit);
}

SourceTab::SourceTab(HWND hParentWindow)
{
	const HINSTANCE hInstance = GetModuleHandle(nullptr);

	this->m_hWndParent = hParentWindow;

	if (FAILED(RegisterSourceTabWindowClass(hInstance)))
	{
		Logger::Write(L"Failed to register source tab window class!");
		return;
	}

	if (FAILED(InitializeSourceTabWindow(hInstance)))
	{
		Logger::Write(L"Failed to initialize source tab window");
		return;
	}
}

static HRESULT RegisterSourceTabWindowClass(HINSTANCE hInstance)
{
	static bool hasBeenRegistered = false;

	if (!hasBeenRegistered)
	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(WNDCLASSEX));
		wcex.cbSize = sizeof(wcex);
		wcex.lpszClassName = SOURCE_TAB_CLASS;
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpfnWndProc = ::SourceTabWindowProcedure;
		wcex.cbWndExtra = sizeof(SourceTab*);
		wcex.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcex))
			return E_FAIL;

		hasBeenRegistered = true;
	}

	return S_OK;
}

HRESULT SourceTab::InitializeSourceTabWindow(HINSTANCE hInstance)
{
	m_hWndSelf = CreateWindow(
		SOURCE_TAB_CLASS,
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

	m_sInfo.lpszFileName = L"Untitled";
	m_sInfo.m_pSourceEdit = new SourceEdit(m_hWndParent); 

	m_hCloseButton = CreateWindow(
		L"Button",
		nullptr,
		WS_CHILD | BS_PUSHBUTTON | BS_OWNERDRAW,
		0, 0, 0, 0,
		m_hWndSelf,
		reinterpret_cast<HMENU>(IDC_CLOSE_BUTTON),
		hInstance,
		nullptr
	);

	return S_OK;
}

void SourceTab::SetName(LPCWSTR lpszName)
{
	m_sInfo.lpszFileName = lpszName;
	SetWindowText(m_hWndSelf, lpszName);
}

LRESULT SourceTab::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		return OnPaint(hWnd);

	case WM_LBUTTONDOWN:
		return OnLButtonDown(hWnd);

	case WM_ERASEBKGND:
		return OnEraseBackground(hWnd, wParam);

	case WM_DRAWITEM:
		return OnDrawItem(hWnd, lParam);

	case WM_MOUSEMOVE:
		return OnMouseMove(hWnd);

	case WM_MOUSELEAVE:
		return OnMouseLeave(hWnd);

	case WM_SIZE:
		return OnSize(hWnd);

	case WM_COMMAND:
		return OnCommand(hWnd, wParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT SourceTab::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

	COLORREF crBackground;

	if (m_IsSelected)
	{
		crBackground = RGB(255, 255, 255);
	}

	else if (m_IsTrackingMouse)
	{
		crBackground = RGB(230, 230, 230);
	}

	else
	{
		crBackground = RGB(210, 210, 210);
	}

	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SelectObject(hDC, GetStockObject(DC_PEN));
	SetDCBrushColor(hDC, crBackground);
	SetDCPenColor(hDC, crBackground);
	Rectangle(hDC, 0, 0, m_rcSelf.right, m_rcSelf.bottom);

	SelectObject(hDC, Utility::GetStandardFont());

	const int length = lstrlen(m_sInfo.lpszFileName);
	SIZE size;
	GetTextExtentPoint32(hDC, m_sInfo.lpszFileName, length, &size);
	SetBkMode(hDC, TRANSPARENT);
	TextOut(
		hDC,
		(m_rcSelf.right - size.cx) / 2,
		(m_rcSelf.bottom - size.cy) / 2,
		m_sInfo.lpszFileName,
		length
	);

	EndPaint(hWnd, &ps);
	return 0;
}

LRESULT SourceTab::OnMouseMove(HWND hWnd)
{
	if (!m_IsTrackingMouse)
	{
		TRACKMOUSEEVENT tme;
		ZeroMemory(&tme, sizeof(tme));
		tme.cbSize = sizeof(tme);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = hWnd;
		TrackMouseEvent(&tme);

		m_IsTrackingMouse = true;

		InvalidateRect(hWnd, NULL, FALSE);
		ShowWindow(m_hCloseButton, SW_SHOW);
	}

	return 0;
}

LRESULT SourceTab::OnMouseLeave(HWND hWnd)
{
	RECT rcWindow;
	POINT pt;

	GetWindowRect(m_hCloseButton, &rcWindow);
	GetCursorPos(&pt);

	if (!PtInRect(&rcWindow, pt))
	{
		ShowWindow(m_hCloseButton, SW_HIDE);
		InvalidateRect(hWnd, NULL, FALSE);
	}

	m_IsTrackingMouse = false;

	return 0;
}

LRESULT SourceTab::OnDrawItem(HWND hWnd, LPARAM lParam)
{
	LPDRAWITEMSTRUCT lpDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	SetDCBrushColor(lpDIS->hDC, RGB(0xFF, 0x00, 0x00));
	SetDCPenColor(lpDIS->hDC, RGB(0xFF, 0x00, 0x00));

	SelectObject(lpDIS->hDC, GetStockObject(DC_BRUSH));
	SelectObject(lpDIS->hDC, GetStockObject(DC_PEN));

	/* Background */
	Rectangle(lpDIS->hDC, 0, 0, lpDIS->rcItem.right, lpDIS->rcItem.bottom);

	SetTextColor(lpDIS->hDC, 0xFFFFFF);
	SetBkMode(lpDIS->hDC, TRANSPARENT);
	
	SetDCPenColor(lpDIS->hDC, 0xFFFFFF);

	/* Draw line top left to bottom right */
	MoveToEx(lpDIS->hDC, 4, 4, NULL);
	LineTo(lpDIS->hDC, lpDIS->rcItem.right - 4, lpDIS->rcItem.bottom - 4);

	/* Draw line bottom left to top right*/
	MoveToEx(lpDIS->hDC, 4, lpDIS->rcItem.bottom - 1 - 4, NULL);
	LineTo(lpDIS->hDC, lpDIS->rcItem.right - 1 - 3, 3);

	return 0;
}

LRESULT SourceTab::OnCommand(HWND hWnd, WPARAM wParam)
{
	if (wParam == IDC_CLOSE_BUTTON)
	{
		SendMessage(m_hWndParent, WM_CLOSE_TAB, NULL, reinterpret_cast<LPARAM>(this));
	}

	return 0;
}

LRESULT SourceTab::OnSize(HWND hWnd)
{
	const int iButtonSize = static_cast<int>(m_rcSelf.bottom * 0.6);

	int distance = (m_rcSelf.bottom - iButtonSize) / 2;

	SetWindowPos(
		m_hCloseButton,
		nullptr,
		m_rcSelf.right - distance - iButtonSize,
		distance,
		iButtonSize,
		iButtonSize,
		SWP_NOZORDER
	);

	return 0;
}

LRESULT SourceTab::OnLButtonDown(HWND hWnd)
{
	WorkArea* pWorkArea = GetAssociatedObject<WorkArea>(GetParent(hWnd));

	if (pWorkArea)
	{
		pWorkArea->UnselectAllTabs();
	}

	this->Select();

	return 0;
}

void SourceTab::Select(void)
{
	if (!m_IsSelected)
	{
		HWND hEditWindow = m_sInfo.m_pSourceEdit->GetHandle();

		RECT rcParent;
		GetClientRect(m_hWndParent, &rcParent);
		SetWindowPos(hEditWindow, nullptr, 0, m_rcSelf.bottom, rcParent.right, rcParent.bottom, SWP_NOZORDER);

		ShowWindow(hEditWindow, SW_SHOW);

		m_IsSelected = true;
		InvalidateRect(m_hWndSelf, NULL, TRUE);
	}
}

void SourceTab::Unselect(void)
{
	if (m_IsSelected)
	{
		ShowWindow(m_sInfo.m_pSourceEdit->GetHandle(), SW_HIDE);

		m_IsSelected = false;
		InvalidateRect(m_hWndSelf, NULL, TRUE);
	}
}

bool SourceTab::IsSelected(void)
{
	return m_IsSelected;
}

LRESULT SourceTab::OnEraseBackground(HWND hWnd, WPARAM wParam)
{
	return FALSE;
}

static LRESULT OnCreate(HWND hWnd, LPARAM lParam)
{
	SetWindowLongPtr(
		hWnd, GWLP_USERDATA,
		reinterpret_cast<LONG>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams)
	);

	return 0;
}

static LRESULT CALLBACK SourceTabWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return OnCreate(hWnd, lParam);

	default:
		SourceTab* pSourceTab = GetAssociatedObject<SourceTab>(hWnd);

		if (pSourceTab != nullptr)
		{
			return pSourceTab->WindowProcedure(hWnd, uMsg, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}