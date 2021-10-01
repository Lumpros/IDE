#include "Window.h"

HWND Window::GetHandle(void) const
{
	return m_hWndSelf;
}

RECT Window::GetRect(void) const
{
	return m_rcSelf;
}

void Window::SetSize(int cx, int cy)
{
	m_rcSelf.right = cx;
	m_rcSelf.bottom = cy;

	SetWindowPos(m_hWndSelf,
		nullptr,
		NULL, 
		NULL,
		cx,
		cy,
		SWP_NOZORDER | SWP_NOMOVE
	);
}

void Window::SetPos(int x, int y)
{
	m_rcSelf.left = x;
	m_rcSelf.top = y;

	SetWindowPos(m_hWndSelf,
		nullptr,
		x,
		y,
		NULL, 
		NULL,
		SWP_NOZORDER | SWP_NOSIZE
	);
}

void Window::Show(void)
{
	ShowWindow(m_hWndSelf, SW_SHOW);
}

void Window::Hide(void)
{
	ShowWindow(m_hWndSelf, SW_HIDE);
}

bool Window::IsVisible(void) const
{
	return GetWindowLong(m_hWndSelf, GWL_STYLE) & WS_VISIBLE;
}