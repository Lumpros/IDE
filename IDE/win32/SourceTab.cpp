#include "SourceTab.h"
#include "Logger.h"
#include "Utility.h"
#include "WorkArea.h"
#include "AppWindow.h"


#include <fstream>
#include <sstream>
#include <codecvt>

#define SOURCE_TAB_CLASS L"IDESourceTabClass"

#define IDC_CLOSE_BUTTON 100

static HRESULT RegisterSourceTabWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK SourceTabWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

SourceTab::~SourceTab(void)
{
	SAFE_DELETE_PTR(m_sInfo.m_pSourceEdit);

	free(m_sInfo.lpszFileName);
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
		L"Untitled",
		WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		m_hWndParent,
		nullptr,
		hInstance,
		this
	);

	if (!m_hWndSelf)
		return E_FAIL;

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
	if (m_sInfo.lpszFileName != nullptr)
	{
		free(m_sInfo.lpszFileName);
	}

	const size_t length = lstrlen(lpszName);

	m_sInfo.lpszFileName = _wcsdup(lpszName);

	if (lpszName[length - 1] == L'*') {
		m_sInfo.lpszFileName[length - 1] = L'\0';
	}

	std::wstring file_name = Utility::GetFileNameFromPath(m_sInfo.lpszFileName);

	if (this->m_sInfo.m_pSourceEdit->HasBeenEdited())
		file_name.push_back(L'*');

	SetWindowText(m_hWndSelf, file_name.c_str());

	InvalidateRect(m_hWndSelf, NULL, FALSE);
}

void SourceTab::RemoveAsteriskFromDisplayedName(void)
{
	const size_t windowTextLength = GetWindowTextLength(m_hWndSelf) + 1;

	wchar_t* buffer = new wchar_t[windowTextLength];

	if (buffer != nullptr)
	{
		GetWindowText(m_hWndSelf, buffer, windowTextLength);

		const size_t len = lstrlen(buffer);

		if (buffer[len - 1] == L'*')
		{
			buffer[len - 1] = L'\0';
			SetWindowText(m_hWndSelf, buffer);
			InvalidateRect(m_hWndSelf, NULL, FALSE);
		}

		delete[] buffer;
	}
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
		crBackground = RGB(192, 192, 192);
	}

	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SelectObject(hDC, GetStockObject(DC_PEN));
	SetDCBrushColor(hDC, crBackground);
	SetDCPenColor(hDC, crBackground);
	Rectangle(hDC, 0, 0, m_rcSelf.right, m_rcSelf.bottom);

	SelectObject(hDC, Utility::GetStandardFont());
	SetBkMode(hDC, TRANSPARENT);

	int nMaxCount = GetWindowTextLength(hWnd) + 1;
	LPWSTR lpWindowText = new wchar_t[nMaxCount];
	GetWindowText(hWnd, lpWindowText, nMaxCount);

	SIZE size;
	GetTextExtentPoint32(hDC, lpWindowText, lstrlen(lpWindowText), &size);
	TextOut(hDC, 
		(m_rcSelf.bottom - size.cy),
		(m_rcSelf.bottom - size.cy) / 2,
		lpWindowText, lstrlen(lpWindowText));

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

		ShowWindow(m_hCloseButton, SW_SHOW);
		InvalidateRect(hWnd, NULL, FALSE);
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
		if (!m_IsSelected)
		{
			crButton = RGB(200, 0, 0);
		}

		HideCloseButton();
		InvalidateRect(hWnd, NULL, FALSE);
	}

	m_IsTrackingMouse = false;

	return 0;
}

LRESULT SourceTab::OnDrawItem(HWND hWnd, LPARAM lParam)
{
	LPDRAWITEMSTRUCT lpDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);

	SetDCBrushColor(lpDIS->hDC, crButton);
	SetDCPenColor(lpDIS->hDC, crButton);

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
		crButton = RGB(0xFF, 0, 0);

		HWND hEditWindow = m_sInfo.m_pSourceEdit->GetHandle();

		constexpr int offset = 1;

		AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(GetParent(m_hWndParent));

		RECT rcParent;
		GetClientRect(m_hWndParent, &rcParent);

		RECT rcStatusBar;
		GetClientRect(pAppWindow->GetStatusBar()->GetHandle(), &rcStatusBar);

		m_sInfo.m_pSourceEdit->SetPos(0, m_rcSelf.bottom + offset);
		m_sInfo.m_pSourceEdit->SetSize(
			rcParent.right,
			rcParent.bottom - rcStatusBar.bottom + 3
		);
		m_sInfo.m_pSourceEdit->Show();

		m_IsSelected = true;
		InvalidateRect(m_hWndSelf, NULL, TRUE);

		SendMessage(m_hWndParent, WM_TAB_SELECTED, NULL, (LPARAM)this);
	}
}

void SourceTab::Unselect(void)
{
	if (m_IsSelected)
	{
		ShowWindow(m_sInfo.m_pSourceEdit->GetHandle(), SW_HIDE);
		crButton = RGB(200, 0, 0);
		m_IsSelected = false;
		InvalidateRect(m_hWndSelf, NULL, TRUE);
	}
}

void SourceTab::HideCloseButton(void) const
{
	ShowWindow(m_hCloseButton, SW_HIDE);
}

bool SourceTab::IsSelected(void)
{
	return m_IsSelected;
}

void SourceTab::SetEditTextToContentsOfFile(LPCWSTR lpPath)
{
	std::wifstream file(lpPath);
	file.imbue(std::locale(std::locale::empty(), new std::codecvt_utf8<wchar_t>));

	std::wstringstream buffer;
	buffer << file.rdbuf();

	SetWindowText(m_sInfo.m_pSourceEdit->GetHandle(), buffer.str().c_str());
}

void SourceTab::SetTemporary(bool temporary)
{
	m_IsTemporary = temporary;
}

int SourceTab::GetRequiredTabWidth(void) const
{
	HDC hDC = GetDC(m_hWndSelf);

	const int iButtonSize = static_cast<int>(20 * 0.6 * Utility::GetScaleForDPI(m_hWndSelf));

	const wchar_t* name = GetName();
	SIZE size;
	GetTextExtentPoint32(hDC, name, lstrlen(name), &size);
	delete[] name;
	ReleaseDC(m_hWndSelf, hDC);

	return static_cast<int>(size.cx + iButtonSize * 2 * Utility::GetScaleForDPI(m_hWndSelf));
}

LRESULT SourceTab::OnEraseBackground(HWND hWnd, WPARAM wParam)
{
	return FALSE;
}

bool SourceTab::IsTemporary(void) const
{
	return m_IsTemporary;
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