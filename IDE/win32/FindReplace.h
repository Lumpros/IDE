#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

namespace FR
{
	bool Find(const wchar_t* lpszFind,
		      HWND hEditControl,
		      DWORD dwFlags);

	bool Replace(const wchar_t* lpszReplace,
		         HWND hEditControl,
		         DWORD dwFlags);

	bool ReplaceAll(const wchar_t* lpszFind,
		            const wchar_t* lpszReplace,
		            HWND hEditControl,
		            DWORD dwFlags);
}