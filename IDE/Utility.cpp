#include "Utility.h"

#include <stdlib.h>

HFONT hFont = nullptr;

void Utility::CenterWindowRelativeToParent(HWND hWnd)
{
	HWND hParentWindow = GetParent(hWnd);
	RECT rcParentClient;

	/* Parent is the desktop */
	if (hParentWindow == NULL)
	{
		SetRect(&rcParentClient, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
	}

	else
	{
		GetClientRect(hParentWindow, &rcParentClient);
	}

	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	SetWindowPos(
		hWnd,
		nullptr,
		(rcParentClient.right - rcClient.right) / 2, 
		(rcParentClient.bottom - rcClient.bottom) / 2, 
		NULL,
		NULL,
		SWP_NOZORDER | SWP_NOSIZE
	);
}

bool Utility::IsNumber(const wchar_t* lpszText)
{
	const int value = _wtoi(lpszText);
	const size_t length = lstrlen(lpszText);

	if (value == 0)
	{
		for (size_t i = 0; i < length; ++i)
		{
			if (lpszText[i] != L'0')
			{
				return false;
			}
		}
	}

	return true;
}

float Utility::GetScaleForDPI(HWND hWnd)
{
	return GetDpiForWindow(hWnd) / 96.0f;
}

HFONT Utility::GetStandardFont(void)
{
	return hFont;
}

void Utility::UpdateFont(HWND hWnd)
{
	if (hFont != nullptr) {
		DeleteObject(hFont);
		hFont = nullptr;
	}

	hFont = CreateFont(
		GetStandardFontHeight(hWnd),
		0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, 0, 0, 0, 0, 0, L"Segoe UI"
	);
}

int Utility::GetStandardFontHeight(HWND hWnd)
{
	return static_cast<int>(16 * GetScaleForDPI(hWnd));
}