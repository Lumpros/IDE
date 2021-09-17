#include "Output.h"

#include <ctime>
#include <strsafe.h>
#include <CommCtrl.h>
#include <Richedit.h>
#include <stdarg.h>
#include <stdio.h>

Output::Output(HWND hParentWindow)
{
	m_hWndParent = hParentWindow;

	m_hWndSelf = CreateWindow(
		MSFTEDIT_CLASS,
		nullptr,
		WS_CHILD | ES_READONLY,
		0, 0, 0, 0,
		m_hWndParent,
		nullptr,
		GetModuleHandle(NULL),
		nullptr
	);

	this->WriteLine(L"Hello, world!");
}

void Output::Clear(void)
{
	SetWindowText(m_hWndSelf, L"");
}

static void AppendText(HWND hWnd, const wchar_t* text)
{
	int iOutLength = GetWindowTextLength(hWnd) + lstrlen(text) + 1;

	wchar_t* buffer = (wchar_t*)GlobalAlloc(GPTR, iOutLength * sizeof(wchar_t));

	if (buffer)
	{
		GetWindowText(hWnd, buffer, iOutLength);
		wcscat_s(buffer, iOutLength, text);
		SetWindowText(hWnd, buffer);

		GlobalFree(buffer);
	}
}

void Output::WriteLine(const wchar_t* lpszFormat, ...)
{
	Write(lpszFormat);
	AppendText(m_hWndSelf, L"\r\n");
}

void Output::Write(const wchar_t* lpszFormat, ...)
{
	va_list arglist;
	va_start(arglist, lpszFormat);

	size_t size = lstrlen(lpszFormat) * 2;
	wchar_t* buffer = new wchar_t[size];
	
	StringCbVPrintf(
		buffer,
		size * sizeof(wchar_t),
		lpszFormat,
		arglist
	);

	AppendText(m_hWndSelf, buffer);

	delete[] buffer;

	va_end(arglist);
}