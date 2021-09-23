#include "Utility.h"

#include <stdlib.h>
#include <CommCtrl.h>

#define DOT_NOT_FOUND -1

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

std::wstring Utility::GetFileExtension(const wchar_t* lpszFileName)
{
	const size_t length = lstrlen(lpszFileName);
	int dot_index = DOT_NOT_FOUND;

	std::wstring extension;

	for (size_t i = 0; i < length; ++i)
	{
		if (lpszFileName[i] == L'.')
		{
			dot_index = i;
			break;
		}
	}

	if (dot_index != DOT_NOT_FOUND)
	{
		extension.assign(lpszFileName + dot_index, lpszFileName + length);
	}

	return extension;
}

void Utility::DrawTextCentered(HDC hDC, const RECT& rc, const wchar_t* lpszText)
{
	const int length = lstrlen(lpszText);
	SIZE size;
	GetTextExtentPoint32(hDC, lpszText, length, &size);

	TextOut(
		hDC,
		(rc.right - size.cx) / 2,
		(rc.bottom - size.cy) / 2,
		lpszText,
		length
	);
}

std::wstring Utility::GetFileNameFromPath(const std::wstring& path)
{
	const size_t position = path.find_last_of(L'\\');

	if (position != std::wstring::npos)
	{
		return path.substr(position + 1, path.size() - position);
	}

	return path;
}

bool Utility::IsPathDirectory(const std::wstring& path)
{
	struct _stat64i32 s;
	_wstat64i32(path.c_str(), &s);

	return s.st_mode & S_IFDIR;
}