#pragma once

#include "Window.h"

class StatusBar : public Window
{
private:

public:
	StatusBar(HWND hParentWindow);

	void SetText(const wchar_t* lpszText, int index);
};

