#include "SourceEdit.h"
#include "WorkArea.h"
#include "ColorFormatParser.h"
#include "Utility.h"
#include "AppWindow.h"
#include "resource.h"

#include <Richedit.h>
#include <CommCtrl.h>
#include <cctype>

#ifndef IsKeyPressed
#define IsKeyPressed(x) (GetKeyState(x) & 0x8000)
#endif

#define NUMBER_BUFSIZ 32
#define BASE10 10

static bool g_hasBeenParsed = false;
static ColorFormatParser g_KeywordColorParser;

static void MarkSourceAsEdited(SourceEdit* pSourceEdit)
{
	if (pSourceEdit != nullptr)
	{
		if (!pSourceEdit->HasBeenEdited())
		{
			SourceTab* pTab = GetAssociatedObject<WorkArea>(GetParent(pSourceEdit->GetHandle()))->GetSelectedTab();

			if (pTab != nullptr)
			{
				const HWND hTabWindow = pTab->GetHandle();
				const size_t length = GetWindowTextLength(hTabWindow) + 2;

				wchar_t* buffer = new wchar_t[length];

				if (buffer != nullptr)
				{
					GetWindowText(hTabWindow, buffer, length);

					const size_t strlength = lstrlen(buffer);

					buffer[strlength] = L'*';
					buffer[strlength + 1] = L'\0';

					SetWindowText(hTabWindow, buffer);
					InvalidateRect(hTabWindow, NULL, FALSE);

					pSourceEdit->MarkAsEdited();

					delete[] buffer;
				}
			}
		}
	}
}

static LRESULT OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam, DWORD_PTR dwRefData)
{
	LRESULT result = DefSubclassProc(hWnd, WM_CHAR, wParam, lParam);

	SourceEdit* pSourceEdit = reinterpret_cast<SourceEdit*>(dwRefData);

	if (!IsKeyPressed(VK_CONTROL))
	{
		::MarkSourceAsEdited(pSourceEdit);
	}

	Utility::UpdateUndoMenuButton(hWnd);

	return result;
}

static inline int GetLineCount(HWND hWnd)
{
	return SendMessage(hWnd, EM_GETLINECOUNT, NULL, NULL);
}

/// <summary>
/// Calculates and returns the height of a standard edit control line in pixels
/// </summary>
/// <param name="hWnd"></param>
/// <returns></returns>
static int GetLineHeight(HWND hWnd)
{
	return Utility::GetStandardFontHeight(GetAncestor(hWnd, GA_ROOT));
}

static double GetZoomScale(HWND hWnd)
{
	int numerator = 0, denominator = 0;

	SendMessage(hWnd,
		        EM_GETZOOM,
		        reinterpret_cast<WPARAM>(&numerator),
		        reinterpret_cast<LPARAM>(&denominator));

	if (0 == denominator)
	{
		numerator = denominator = 1;
	}

	return numerator / static_cast<double>(denominator);
}

static void DrawLineNumberingBackground(HDC hDC, HWND hWnd, const RECT* p_rcClient)
{
	double zoom_scale = ::GetZoomScale(hWnd);

	const int left_margin = static_cast<const int>(LOWORD(SendMessage(hWnd, EM_GETMARGINS, NULL, NULL)) * zoom_scale);

	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SelectObject(hDC, GetStockObject(DC_PEN));
	SetDCBrushColor(hDC, RGB(230, 230, 230));
	SetDCPenColor(hDC, RGB(230, 230, 230));

	Rectangle(hDC, p_rcClient->left, 0, left_margin + p_rcClient->left, p_rcClient->bottom);
}

static int CalculateLineNumberContainerHorizontalOffset(HWND hWnd)
{
	SCROLLINFO sInfo = {};
	sInfo.cbSize = sizeof(SCROLLINFO);
	sInfo.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_HORZ, &sInfo);

	return -(sInfo.nTrackPos - sInfo.nPos);
}

static int CalculateLineNumberContainerVerticalOffset(HWND hWnd, int iLineHeight)
{
	SCROLLINFO sInfo = {};
	sInfo.cbSize = sizeof(SCROLLINFO);
	sInfo.fMask = SIF_ALL;
	GetScrollInfo(hWnd, SB_VERT, &sInfo);

	int offset = 0;

	if (sInfo.nTrackPos != 0)
	{
		offset = (sInfo.nTrackPos - sInfo.nPos);
	}

	return -offset % iLineHeight;
}

static void DrawLineNumbers(HDC hDC, HWND hWnd, const RECT* p_rcRect)
{
	double zoom_scale = ::GetZoomScale(hWnd);

	HFONT hFont = CreateFont(
		(int)(Utility::GetStandardFontHeight(hWnd) * zoom_scale),
		0, 0, 0, FW_NORMAL, 0, 0, 0, 0, 0, 0, 0, 0, L"Segoe UI"
	);

	SetBkMode(hDC, TRANSPARENT);
	SelectObject(hDC, hFont);
	SetTextColor(hDC, RGB(150, 150, 150));

	const int iLineCount = static_cast<int>(GetLineCount(hWnd));
	const int iLineHeight = static_cast<int>(GetLineHeight(hWnd) * zoom_scale);
	const int left_margin = static_cast<int>(LOWORD(SendMessage(hWnd, EM_GETMARGINS, NULL, NULL)) * zoom_scale);
	const int iDistanceFromRightEdge = static_cast<int>(left_margin * 0.1);

	int x = p_rcRect->left;
	int y = ::CalculateLineNumberContainerVerticalOffset(hWnd, iLineHeight);

	for (int i = SendMessage(hWnd, EM_GETFIRSTVISIBLELINE, 0, 0); i < iLineCount; ++i)
	{
		wchar_t buf[NUMBER_BUFSIZ];
		_itow_s(i + 1, buf, NUMBER_BUFSIZ, BASE10);

		RECT rc = { x, y, left_margin - iDistanceFromRightEdge + x, iLineHeight * (i + 1) };

		DrawText(hDC, buf, lstrlen(buf), &rc, DT_RIGHT);

		y += iLineHeight;

		if (!PtInRect(p_rcRect, { 1, y }))
		{
			break;
		}
	}

	DeleteObject(hFont);
}

/// <summary>
/// Paints the background of the left margin and then
/// paints the numbers of the current lines on screen
/// </summary>
/// <param name="hWnd"> Handle to the edit control </param>
static LRESULT OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = DefSubclassProc(hWnd, WM_PAINT, wParam, lParam);

	HDC hDC = GetDC(hWnd);

	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	rcClient.left = ::CalculateLineNumberContainerHorizontalOffset(hWnd);

	DrawLineNumberingBackground(hDC, hWnd, &rcClient);
	DrawLineNumbers(hDC, hWnd, &rcClient);

	ReleaseDC(hWnd, hDC);

	return result;
}

static LRESULT OnKeyDown(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	/* Remove formatting keys */
	if (IsKeyPressed(VK_CONTROL))
	{
		switch (wParam)
		{
		case L'E': case 'R':
		case L'J': case 'L':
		case L'1': case '2':
		case L'5':
			return 0;

		case 'V': /* Remove format from pasted text */
			SendMessage(hWnd, EM_PASTESPECIAL, CF_UNICODETEXT, NULL);
			return 0;

		case 'C': {
			LRESULT ret = DefSubclassProc(hWnd, WM_KEYDOWN, wParam, lParam);
			Utility::RefreshPasteMenuButton(GetMenu(GetAncestor(hWnd, GA_ROOT)));
			return ret;
		}

		default:
			if (IsKeyPressed(VK_SHIFT))
			{
				switch (wParam)
				{
				case L'A': case '7':
					return 0;
				}
			}
		}
	}

	return DefSubclassProc(hWnd, WM_KEYDOWN, wParam, lParam);
}

static BOOL SelectionHasChanged(UINT message, LPARAM lParam)
{
	if (message == WM_SETCURSOR)
	{
		return HIWORD(lParam) != WM_MOUSEMOVE;
	}

	return TRUE;
}

// CCD = Copy, Cut, Delete
static void ToggleCCDButtons(HWND hEditWindow, UINT uEnable)
{
	HMENU hMenu = GetMenu(GetAncestor(hEditWindow, GA_ROOT));

	EnableMenuItem(hMenu, ID_EDIT_COPY, uEnable);
	EnableMenuItem(hMenu, ID_EDIT_CUT, uEnable);
	EnableMenuItem(hMenu, ID_EDIT_DELETE, uEnable);
}

LRESULT CALLBACK SourceEditSubclassProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
{
	SourceEdit* pSource = reinterpret_cast<SourceEdit*>(dwRefData);

	switch (uMsg)
	{
	case WM_CHAR:
	case WM_KEYDOWN:
	case WM_KEYUP:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_RBUTTONDBLCLK:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_MBUTTONDBLCLK:
	case EM_SETSEL:
	case EM_EXSETSEL:
		// TODO: refresh copy/cut/replace buttons
		if (SelectionHasChanged(uMsg, lParam)) {
			pSource->RefreshStatusBarText();
			CHARRANGE cr;
			SendMessage(hWnd, EM_EXGETSEL, NULL, reinterpret_cast<LPARAM>(&cr));
			if (cr.cpMin != cr.cpMax)
				ToggleCCDButtons(hWnd, MF_ENABLED);
			else
				ToggleCCDButtons(hWnd, MF_GRAYED);
		}
		break;
	}

	switch (uMsg)
	{
	case WM_CHAR:
		return OnChar(hWnd, wParam, lParam, dwRefData);

	case WM_PAINT:
		return OnPaint(hWnd, wParam, lParam);

	case WM_KEYDOWN:
		return OnKeyDown(hWnd, wParam, lParam);

	case WM_MOUSEWHEEL:
		LRESULT ret = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		pSource->HandleMouseWheel(wParam);
		return ret;
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/// <summary>
/// Parses the color files if it's the first time the function is called
/// and saves them globally, then initialize the edit window 
/// </summary>
/// <param name="hParentWindow"> Handle to the parent window (WorkArea) </param>
SourceEdit::SourceEdit(HWND hParentWindow)
	: m_Zoomer(this)
{
	if (!g_hasBeenParsed)
	{
		g_KeywordColorParser.ParseFile(L"keywords.color");
		g_hasBeenParsed = false;
	}

	m_hWndParent = hParentWindow;

	m_pStatusBar = GetAssociatedObject<AppWindow>(GetAncestor(hParentWindow, GA_ROOT))->GetStatusBar();

	m_Zoomer.AttachStatusBar(m_pStatusBar);

	m_hWndSelf = CreateWindowEx(
		0,
		MSFTEDIT_CLASS,
		L"",
		WS_CHILD | ES_AUTOVSCROLL | ES_MULTILINE |
		WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL | WS_CLIPSIBLINGS,
		0, 0, 0, 0,
		m_hWndParent,
		0,
		GetModuleHandle(NULL),
		0
	);

	AdjustFontForDPI();
	AdjustLeftMarginForDPI();

	SetWindowSubclass(m_hWndSelf, SourceEditSubclassProcedure, NULL, reinterpret_cast<DWORD_PTR>(this));
}

/// <summary>
/// Calculates the width of the left margin of the edit control
/// in which the line numbers are displayed
/// </summary>
/// <param name=""></param>
/// <returns> width of left margin </returns>
int SourceEdit::GetLeftMargin(void) const
{
	return static_cast<int>(40 * Utility::GetScaleForDPI(GetAncestor(m_hWndSelf, GA_ROOT)));
}

/// <summary>
/// Changed the size of the left margin when the DPI changes so
/// that it will fit the resized numbers appropriately
/// </summary>
/// <param name=""></param>
void SourceEdit::AdjustLeftMarginForDPI(void)
{
	const int iLeftMargin = GetLeftMargin();

	SendMessage(m_hWndSelf, EM_SETMARGINS, EC_LEFTMARGIN, iLeftMargin);
}

/// <summary>
/// Resizes the font of the line numbers when the dpi changes
/// </summary>
/// <param name=""></param>
void SourceEdit::AdjustFontForDPI(void)
{
	SAFE_DELETE_GDIOBJ(m_hFont);

	m_hFont = CreateFont(
		Utility::GetStandardFontHeight(GetAncestor(m_hWndSelf, GA_ROOT)),
		0,
		0,
		0,
		FW_NORMAL,
		FALSE,
		FALSE,
		0,
		0,
		0,
		0,
		0,
		0,
		L"Consolas"
	);

	SendMessage(m_hWndSelf, WM_SETFONT, (WPARAM)m_hFont, 0);
}

void SourceEdit::SetLineColumnStatusBar(void)
{
	CHARRANGE cr;
	SendMessage(m_hWndSelf, EM_EXGETSEL, NULL, (LPARAM)&cr);

	LRESULT lLineIndex = SendMessage(m_hWndSelf, EM_EXLINEFROMCHAR, 0, cr.cpMax);
	LRESULT lColumn = cr.cpMax - SendMessage(m_hWndSelf, EM_LINEINDEX, lLineIndex, NULL);

	WCHAR buf[32];
	wsprintf(buf, L" Ln %d, Col %d", lLineIndex + 1, lColumn + 1);

	m_pStatusBar->SetText(buf, 1);
}

void SourceEdit::MarkAsEdited(void)
{
	m_haveContentsBeenEdited = true;
}

void SourceEdit::MarkAsUnedited(void)
{
	m_haveContentsBeenEdited = false;
}

void SourceEdit::RefreshStatusBarText(void)
{
	this->SetLineColumnStatusBar();
	m_Zoomer.SetStatusBarZoomText();
}

void SourceEdit::HandleMouseWheel(WPARAM wParam)
{
	if ((LOWORD(wParam) & MK_CONTROL) == MK_CONTROL)
	{
		m_Zoomer.Update(GET_WHEEL_DELTA_WPARAM(wParam));
	}
}

SourceEdit::~SourceEdit(void)
{
	SAFE_DELETE_GDIOBJ(m_hFont);
}