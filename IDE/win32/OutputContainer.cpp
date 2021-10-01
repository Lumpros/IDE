#include "OutputContainer.h"
#include "Logger.h"
#include "AppWindow.h"
#include "Utility.h"

#include <CommCtrl.h>

#define OUTPUT_CLASS L"IDEOutputWindowClass"

#define IDC_OUTPUT_BUTTON 200
#define IDC_ERROR_LIST_BUTTON 201

static HRESULT RegisterOutputWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK OutputWindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

constexpr COLORREF crButton = RGB(3, 161, 252);

OutputContainer::OutputContainer(HWND hWnd)
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

OutputContainer::~OutputContainer(void)
{
	SAFE_DELETE_PTR(m_pErrorListButton);
	SAFE_DELETE_PTR(m_pOutputButton);
	SAFE_DELETE_PTR(m_pErrorList);
	SAFE_DELETE_PTR(m_pOutput);
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
		wcex.cbWndExtra = sizeof(OutputContainer*);

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

HRESULT OutputContainer::InitializeOutputWindow(HINSTANCE hInstance)
{
	m_hWndSelf = CreateWindow(
		OUTPUT_CLASS,
		nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
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

	m_pErrorList = new ErrorList(m_hWndSelf);
	m_pOutput = new Output(m_hWndSelf);

	return S_OK;
}

LRESULT OutputContainer::WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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

	case WM_COMMAND:
		return OnCommand(hWnd, wParam);

	case WM_DPICHANGED_BEFOREPARENT:
		SetButtonPositions();
		return 0;

	case WM_DRAWITEM:
		return OnDrawItem(hWnd, lParam);

	case WM_NOTIFY:
		return OnNotify(hWnd, lParam);

	case WM_CTLCOLOREDIT:
		return reinterpret_cast<LRESULT>(GetStockObject(WHITE_BRUSH));
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT OutputContainer::OnNotify(HWND hWnd, LPARAM lParam)
{
	LPNMHEADER pHeader = reinterpret_cast<LPNMHEADER>(lParam);

	/* Disable user resizing the first column of the list view */
	if (pHeader->iItem == 0 &&
		pHeader->hdr.code == HDN_BEGINTRACK)
	{
		return TRUE;
	}

	return FALSE;
}

LRESULT OutputContainer::OnDrawItem(HWND hWnd, LPARAM lParam)
{
	if (m_pErrorListButton && m_pOutputButton)
	{
		LPDRAWITEMSTRUCT lpDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
		m_pErrorListButton->Draw(lpDIS);
		m_pOutputButton->Draw(lpDIS);
	}

	return 0;
}

LRESULT OutputContainer::OnCommand(HWND hWnd, WPARAM wParam)
{
	if (m_pErrorList && m_pOutput)
	{
		switch (LOWORD(wParam))
		{
		case IDC_ERROR_LIST_BUTTON:
			m_pErrorList->Show();
			m_pOutput->Hide();
			break;

		case IDC_OUTPUT_BUTTON:
			m_pErrorList->Hide();
			m_pOutput->Show();
			break;
		}
	}
	
	return 0;
}

LRESULT OutputContainer::OnGetMinMaxInfo(HWND hWnd, LPARAM lParam)
{
	LPMINMAXINFO mmi = reinterpret_cast<LPMINMAXINFO>(lParam);

	mmi->ptMinTrackSize.y = this->GetMinimumHeight();

	return 0;
}

LRESULT OutputContainer::OnSize(HWND hWnd, LPARAM lParam)
{
	m_rcSelf.right = LOWORD(lParam);
	m_rcSelf.bottom = HIWORD(lParam);

	return 0;
}

LRESULT OutputContainer::OnNCHitTest(HWND hWnd)
{
	POINT ptCursor;
	GetCursorPos(&ptCursor);

	RECT rcWindow;
	GetWindowRect(hWnd, &rcWindow);

	if (ptCursor.y <= rcWindow.top + 10)
		return HTTOP;

	return HTCLIENT;
}

LRESULT OutputContainer::OnWindowPosChanged(HWND hWnd, LPARAM lParam)
{
	WINDOWPOS* pWindowPos = reinterpret_cast<WINDOWPOS*>(lParam);
	AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(m_hWndParent);

	if (pAppWindow)
	{
		WorkArea* pWorkArea = pAppWindow->GetWorkArea();
		Explorer* pExplorer = pAppWindow->GetExplorer();

		RECT rcParent;
		GetClientRect(m_hWndParent, &rcParent);

		/* If it has just been initialized, set the button positions */
		/* Do this to avoid doing it every time the window moves to avoid flickering */
		if (m_rcSelf.right == 0 && m_rcSelf.bottom == 0)
		{
			SetButtonPositions();
		}

		RestrictButtonSectorSize(pWindowPos, rcParent, pAppWindow->GetStatusBar());
		ResizeWorkArea(pWorkArea, pExplorer, pWindowPos, rcParent);

		ResizeDisplayWindow(m_pErrorList, pWindowPos);
		ResizeDisplayWindow(m_pOutput, pWindowPos);
		
		m_rcSelf.right = pWindowPos->cx;
		m_rcSelf.bottom = pWindowPos->cy;
	}

	return 0;
}

void OutputContainer::ResizeDisplayWindow(Window* pWindow, LPWINDOWPOS pWindowPos)
{
	if (pWindow)
	{
		const int iMinimumHeight = GetMinimumHeight();

		pWindow->SetPos(0, iMinimumHeight);
		pWindow->SetSize(pWindowPos->cx, pWindowPos->cy - iMinimumHeight);
	}
}

void OutputContainer::RestrictButtonSectorSize(LPWINDOWPOS pWindowPos, const RECT& rcParent, StatusBar* pStatusBar)
{
	if (pWindowPos->y < 0 && rcParent.bottom > 0)
	{
		/* This is true only when the window is being restored after being minimized */
		if (-pWindowPos->y != pWindowPos->cy)
		{
			pWindowPos->y = 0;
			pWindowPos->cy = rcParent.bottom - pStatusBar->GetRefreshedRect().bottom;
			this->SetPos(pWindowPos->x, 0);
		}
	}
}

void OutputContainer::ResizeWorkArea(
	WorkArea* pWorkArea,
	Explorer* pExplorer,
	LPWINDOWPOS pWindowPos,
	const RECT& rcParent
)
{
	if (pWorkArea && pExplorer)
	{
		pWorkArea->SetSize(
			rcParent.right - pExplorer->GetRect().right,
			rcParent.bottom - pWindowPos->cy - 3 - (rcParent.bottom - pExplorer->GetRect().bottom)
		);
	}
}

void OutputContainer::SetButtonPositions(void)
{
	const int min_height = GetMinimumHeight();

	if (m_pOutputButton && m_pErrorListButton)
	{
		const int iButtonWidth = static_cast<int>(80 * Utility::GetScaleForDPI(m_hWndParent));

		m_pOutputButton->SetRect(RECT{ 0, 0, iButtonWidth, min_height });
		m_pErrorListButton->SetRect(RECT{ iButtonWidth, 0, iButtonWidth, min_height });
	}
}

LRESULT OutputContainer::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SelectObject(hDC, GetStockObject(DC_PEN));

	SetDCBrushColor(hDC, crButton);
	SetDCPenColor(hDC, crButton);

	const int rect_height = GetMinimumHeight();

	Rectangle(hDC, 0, 0, m_rcSelf.right, rect_height);

	EndPaint(hWnd, &ps);
	return 0;
}

int OutputContainer::GetMinimumHeight(void)
{
	return static_cast<const int>(22 * Utility::GetScaleForDPI(m_hWndParent));
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
		OutputContainer* pOutput = GetAssociatedObject<OutputContainer>(hWnd);

		if (pOutput)
		{
			return pOutput->WindowProcedure(hWnd, uMessage, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}