#include "StatusBar.h"

#include <CommCtrl.h>

StatusBar::StatusBar(HWND hParentWindow)
{
	const HINSTANCE hInstance = GetModuleHandle(NULL);

	m_hWndParent = hParentWindow;

	m_hWndSelf = CreateWindow(
		STATUSCLASSNAME,
		nullptr,
		SBARS_SIZEGRIP | WS_CHILD | WS_VISIBLE,
		0, 0, 0, 0,
		m_hWndParent,
		(HMENU)500,
		hInstance,
		nullptr
	);
}