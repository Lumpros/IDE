#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

namespace FR
{
	bool Find(const wchar_t* lpszFind,
		      HWND hEditControl,
		      DWORD dwFlags);

	void Replace(const wchar_t* lpszReplace,
		         HWND hEditControl,
		         DWORD dwFlags);

	void ReplaceAll(const wchar_t* lpszFind,
		            const wchar_t* lpszReplace,
		            HWND hEditControl,
		            DWORD dwFlags);
}