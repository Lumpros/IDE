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

static LRESULT OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = DefSubclassProc(hWnd, WM_CHAR, wParam, lParam);
	
	return result;
}

static inline int GetLineCount(HWND hWnd)
{
	return SendMessage(hWnd, EM_GETLINECOUNT, NULL, NULL);
}

static int GetLineHeight(HWND hWnd)
{
	return Utility::GetStandardFontHeight(GetAncestor(hWnd, GA_ROOT));
}

static LRESULT OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = DefSubclassProc(hWnd, WM_PAINT, wParam, lParam);

	HDC hDC = GetDC(hWnd);

	SelectObject(hDC, Utility::GetStandardFont());
	SelectObject(hDC, GetStockObject(WHITE_BRUSH));
	SelectObject(hDC, GetStockObject(WHITE_PEN));
	SetTextColor(hDC, RGB(245, 0, 0));

	const int left_margin = LOWORD(SendMessage(hWnd, EM_GETMARGINS, NULL, NULL));
	const int x_offset    = static_cast<const int>(8 * Utility::GetScaleForDPI(hWnd));
	const int iLineCount  = GetLineCount(hWnd);

	int iLineHeight = GetLineHeight(hWnd), y = 0;

	if (Utility::GetScaleForDPI(hWnd) == 1.0f)
	{
		++iLineHeight;
	}

	for (int i = SendMessage(hWnd, EM_GETFIRSTVISIBLELINE, 0, 0); i < iLineCount; ++i)
	{
		Rectangle(hDC, x_offset, y, left_margin, y + iLineHeight);

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
	WPARAM value = wParam;

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
		WS_CHILD | ES_AUTOHSCROLL | ES_AUTOVSCROLL | 
		ES_MULTILINE | WS_VSCROLL | WS_HSCROLL,
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

int SourceEdit::GetLeftMargin(void) const
{
	return static_cast<int>(40 * Utility::GetScaleForDPI(GetAncestor(m_hWndSelf, GA_ROOT)));
}

void SourceEdit::AdjustLeftMarginForDPI(void)
{
	const int iLeftMargin = GetLeftMargin();

	SendMessage(m_hWndSelf, EM_SETMARGINS, EC_LEFTMARGIN, iLeftMargin);
}

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