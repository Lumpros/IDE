#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

namespace FR
{
	enum class SearchDirection {
		UP, DOWN
	};

	void Find(
		_In_ const wchar_t* lpszTarget,
		_In_ HWND hEditControl,
		_In_ SearchDirection dir,
		_In_ bool matchCase,
		_In_ bool wrapAround
	);

	void Replace(
		_In_ const wchar_t* lpszTarget,
		_In_ const wchar_t* lpszNewText,
		_In_ HWND hEditControl,
		_In_ SearchDirection dir,
		_In_ bool matchCase,
		_In_ bool wrapAround
	);
}