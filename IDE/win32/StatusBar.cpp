#include "StatusBar.h"

#include <CommCtrl.h>

#define IDC_STATUS_BAR 500
#define SIZEOF_ARR(arr) sizeof(arr) / sizeof(arr[0])

const int dxStatusWidths[] = { 0, 174, 64, 150, 150, -1 };

int iStatusWidths[SIZEOF_ARR(dxStatusWidths)];

// Function taken from my Notepad project lol
// The first part is the only one that changes size as the window changes size
// If the window is so small that the first part's with is 0, then the rest of
// the parts start becoming smaller
static void SetStatusWidths(HWND hWnd)
{
	int sum = 0;

	// Get the width in pixels of all parts (after the first)
	// So we can deduce how big the first part should be
	for (unsigned int i = 1; i < SIZEOF_ARR(dxStatusWidths); ++i)
		sum += dxStatusWidths[i];

	RECT clientRect;
	GetClientRect(hWnd, &clientRect);

	// Calculate first part width
	int part_width = clientRect.right - sum;

	iStatusWidths[0] = part_width > 0 ? part_width : 0;

	int previousWidthsSum = 0;

	// Apply changes to each part based on the previous calculations
	for (unsigned int i = 1; i < SIZEOF_ARR(dxStatusWidths); ++i)
	{
		previousWidthsSum += dxStatusWidths[i - 1];
		iStatusWidths[i] = dxStatusWidths[i] + part_width + previousWidthsSum;
	}
}

static LRESULT CALLBACK
StatusBarCallback(
	HWND hWnd,
	UINT uMessage,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData
)
{
	switch (uMessage)
	{
	case WM_SIZE:
		SetStatusWidths(GetParent(hWnd));
		SendMessage(hWnd, SB_SETPARTS, SIZEOF_ARR(iStatusWidths), reinterpret_cast<LPARAM>(iStatusWidths));
		break;
	}
	
	return DefSubclassProc(hWnd, uMessage, wParam, lParam);
}

StatusBar::StatusBar(HWND hParentWindow)
{
	const HINSTANCE hInstance = GetModuleHandle(NULL);

	m_hWndParent = hParentWindow;

	m_hWndSelf = CreateWindow(
		STATUSCLASSNAME,
		nullptr,
		SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		m_hWndParent,
		reinterpret_cast<HMENU>(IDC_STATUS_BAR),
		hInstance,
		nullptr
	);

	SetStatusWidths(m_hWndParent);
	SendMessage(m_hWndSelf, SB_SETPARTS, SIZEOF_ARR(iStatusWidths), reinterpret_cast<LPARAM>(iStatusWidths));

	SetText(L"Ln 1 Col 1", 1);
	SetText(L"100%", 2);
	SetText(L"Windows (CRLF)", 3);
	SetText(L"UTF-8", 4);

	ClearEditorInformation();

	SetWindowSubclass(m_hWndSelf, StatusBarCallback, NULL, NULL);
}

void StatusBar::SetText(const wchar_t* lpszText, int index)
{
	SendMessage(m_hWndSelf, SB_SETTEXT, index, reinterpret_cast<LPARAM>(lpszText));
}

RECT StatusBar::GetRefreshedRect(void) const
{
	if (!IsVisible())
		return RECT{ 0, 0, 0, 0 };

	RECT rcClient;
	GetClientRect(m_hWndSelf, &rcClient);

	return rcClient;
}

void StatusBar::ClearEditorInformation(void)
{
	for (int i = 1; i <= SIZEOF_ARR(iStatusWidths); ++i)
	{
		SetText(L"", i);
	}
}