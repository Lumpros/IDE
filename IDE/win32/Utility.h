#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

#include <string>

#define SAFE_DELETE_PTR(ptr) if (ptr) { delete ptr; ptr = nullptr; }
#define SAFE_DELETE_GDIOBJ(obj) if (obj) { DeleteObject(obj); obj = nullptr; }

template <typename T> 
inline T* GetAssociatedObject(HWND hWnd)
{
	return reinterpret_cast<T*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

namespace Utility
{
	extern void CenterWindowRelativeToParent(HWND hWnd);
	extern bool IsNumber(const wchar_t* lpszText);
	extern float GetScaleForDPI(HWND hWnd);
	extern HFONT GetStandardFont(void);
	extern int GetStandardFontHeight(HWND hWnd);
	extern void UpdateFont(HWND hWnd);
	extern std::wstring GetFileExtension(const wchar_t* lpszFileName);
	extern void DrawTextCentered(HDC hDC, const RECT& rc, const wchar_t* lpszText);
}