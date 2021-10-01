#pragma once

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

class Window
{
protected:
	HWND m_hWndSelf = nullptr;
	HWND m_hWndParent = nullptr;
	RECT m_rcSelf = { 0 };

public:
	HWND GetHandle(void) const;
	RECT GetRect(void) const;

	void SetSize(int cx, int cy);
	void SetPos(int x, int y);

	void Show(void);
	void Hide(void);

	bool IsVisible(void) const;
};

