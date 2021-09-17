#include "Output.h"
#include "Utility.h"

#include <CommCtrl.h>

struct ListViewColumn
{
	int cx;
	const wchar_t* lpszName;
};

ListViewColumn columns[] = {
		{ 30, L" " },
		{ 50, L"Code" },
		{ 400, L"Description" },
		{ 100, L"File" },
		{ 50, L"Line" },
};

LRESULT CALLBACK ListViewSubclassProcedure(
	HWND hWnd,
	UINT uMessage,
	WPARAM wParam,
	LPARAM lParam,
	UINT_PTR uIdSubclass,
	DWORD_PTR dwRefData)
{
	switch (uMessage)
	{
	case WM_DPICHANGED_BEFOREPARENT:
		((Output*)(dwRefData))->OnDPIChange();
		break;
	}

	return DefSubclassProc(hWnd, uMessage, wParam, lParam);
}

Output::Output(HWND hParentWindow)
{
	m_hWndParent = hParentWindow;
	
	m_hWndSelf = CreateWindow(
		WC_LISTVIEW,
		L"",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
		0, 0, 0, 0,
		m_hWndParent,
		nullptr,
		GetModuleHandle(nullptr),
		nullptr
	);

	SetWindowSubclass(m_hWndSelf, ListViewSubclassProcedure, NULL, reinterpret_cast<DWORD_PTR>(this));

	InitializeListViewColumns();
	UpdateListViewColumnWidths();

	return;

	LVITEM lvi;
	lvi.pszText = (wchar_t*)L"a value of type \"const wchar_t*\" cannot be assigned to an entity of type \"LPWSTR\"";
	lvi.mask = LVIF_TEXT;
	lvi.state = lvi.iSubItem = lvi.stateMask = 0;
	ListView_InsertItem(m_hWndSelf, &lvi);
}

void Output::InitializeListViewColumns(void)
{
	wchar_t szText[256];

	LVCOLUMN lvc;
	lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

	for (int iCol = 0; iCol < sizeof(columns) / sizeof(ListViewColumn); ++iCol)
	{
		lvc.iSubItem = iCol;
		lvc.pszText = szText;
		lvc.cx = 0;
		lvc.fmt = LVCFMT_LEFT;
		lstrcpy(szText, columns[iCol].lpszName);

		ListView_InsertColumn(m_hWndSelf, iCol, &lvc);
	}
}

void Output::UpdateListViewColumnWidths(void)
{
	const float dpiScale = Utility::GetScaleForDPI(m_hWndParent);

	for (int iCol = 0; iCol < sizeof(columns) / sizeof(ListViewColumn); ++iCol)
	{
		ListView_SetColumnWidth(m_hWndSelf, iCol, static_cast<int>(columns[iCol].cx * dpiScale));
	}
}

void Output::OnDPIChange(void)
{
	UpdateListViewColumnWidths();
}