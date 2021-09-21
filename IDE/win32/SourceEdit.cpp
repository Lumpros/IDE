#include "SourceEdit.h"

#include <Richedit.h>
#include <CommCtrl.h>
#include <cctype>

#include "ColorFormatParser.h"
#include "Utility.h"

#ifndef IsKeyPressed
#define IsKeyPressed(x) (GetKeyState(x) & 0x8000)
#endif

#define NUMBER_BUFSIZ 32
#define BASE10 10

static bool g_hasBeenParsed = false;
static ColorFormatParser g_KeywordColorParser;

/// <summary>
/// When a character is entered in the edit control, checks
/// if a keyword was written, and if so, sets the appropriate color
/// </summary>
/// <param name="hWnd"> Handle to the rich edit control </param>
static LRESULT OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = DefSubclassProc(hWnd, WM_CHAR, wParam, lParam);

	return result;

	DWORD dwStart, dwEnd, dwOldStart;
	SendMessage(hWnd, EM_GETSEL, (WPARAM)&dwStart, (LPARAM)&dwEnd);

	dwOldStart = dwStart;

	const int max_length = g_KeywordColorParser.GetMaxLength();

	wchar_t* buf = new wchar_t[max_length + 1];

	int strOffset = 0;

	for (int i = 0; i < max_length; ++i)
	{
		if (dwStart == 0)
		{
			break;
		}

		--dwStart;

		SendMessage(hWnd, EM_SETSEL, dwStart, dwEnd);
		SendMessage(hWnd, EM_GETSELTEXT, 0, (LPARAM)buf);

		if (iswspace(buf[0]) || (iswpunct(buf[0]) && buf[0] != L'#'))
		{
			++dwStart;
			strOffset = 1;
			break;
		}
	}

	SendMessage(hWnd, EM_SETSEL, dwStart, dwEnd);

	CRSTATUS crStatus = g_KeywordColorParser.GetKeywordColor(buf + strOffset);

	CHARFORMATW cf;
	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.crTextColor = 0;
	cf.dwEffects = 0;

	if (crStatus.wasFound)
	{
		cf.crTextColor = crStatus.cr;
	}

	else
	{
		SendMessage(hWnd, EM_SETSEL, dwStart - 1, dwEnd);
	}

	SendMessage(hWnd, EM_SETCHARFORMAT, SCF_NOKBUPDATE | SCF_SELECTION, reinterpret_cast<LPARAM>(&cf));

	SendMessage(hWnd, EM_SETSEL, dwOldStart, dwEnd);

	delete[] buf;

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

/// <summary>
/// Paints the background of the left margin and then
/// paints the numbers of the current lines on screen
/// </summary>
/// <param name="hWnd"> Handle to the edit control </param>
static LRESULT OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = DefSubclassProc(hWnd, WM_PAINT, wParam, lParam);

	HDC hDC = GetDC(hWnd);

	SelectObject(hDC, Utility::GetStandardFont());
	SelectObject(hDC, GetStockObject(WHITE_BRUSH));
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	SetTextColor(hDC, RGB(150, 150, 150));

	const int left_margin = LOWORD(SendMessage(hWnd, EM_GETMARGINS, NULL, NULL));
	const int x_offset    = static_cast<const int>(8 * Utility::GetScaleForDPI(hWnd));
	const int iLineCount  = GetLineCount(hWnd);

	int iLineHeight = GetLineHeight(hWnd), y = 0;

	if (Utility::GetScaleForDPI(hWnd) == 1.0f)
	{
		++iLineHeight;
	}

	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	SelectObject(hDC, GetStockObject(DC_BRUSH));
	SelectObject(hDC, GetStockObject(DC_PEN));
	SetDCBrushColor(hDC, RGB(230, 230, 230));
	SetDCPenColor(hDC, RGB(230, 230, 230));

	Rectangle(hDC, 0, y, left_margin, rcClient.bottom);

	SetBkMode(hDC, TRANSPARENT);

	for (int i = SendMessage(hWnd, EM_GETFIRSTVISIBLELINE, 0, 0); i < iLineCount; ++i)
	{
		wchar_t buf[NUMBER_BUFSIZ];
		_itow_s(i + 1, buf, NUMBER_BUFSIZ, BASE10);
		TextOut(hDC, x_offset, y, buf, lstrlen(buf));

		y += iLineHeight;
	}

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

static LRESULT OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	return DefSubclassProc(hWnd, WM_COMMAND, wParam, lParam);
}

LRESULT CALLBACK SourceEditSubclassProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_CHAR:
		return OnChar(hWnd, wParam, lParam);

	case WM_PAINT:
		return OnPaint(hWnd, wParam, lParam);

	case WM_KEYDOWN:
		return OnKeyDown(hWnd, wParam, lParam);

	case WM_COMMAND:
		return OnCommand(hWnd, wParam, lParam);
	}

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

/// <summary>
/// Parses the color files if it's the first time the function is called
/// and saves them globally, then initialize the edit window 
/// </summary>
/// <param name="hParentWindow"> Handle to the parent window (WorkArea) </param>
SourceEdit::SourceEdit(HWND hParentWindow)
{
	if (!g_hasBeenParsed)
	{
		g_KeywordColorParser.ParseFile(L"keywords.color");
		g_hasBeenParsed = false;
	}

	m_hWndParent = hParentWindow;

	m_hWndSelf = CreateWindowEx(
		0,
		MSFTEDIT_CLASS,
		L"",
		WS_CHILD | ES_AUTOVSCROLL | ES_MULTILINE |
		WS_VSCROLL | WS_HSCROLL | ES_AUTOHSCROLL,
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

SourceEdit::~SourceEdit(void)
{
	SAFE_DELETE_GDIOBJ(m_hFont);
}