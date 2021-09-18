#pragma once

#include "Window.h"

class Output : public Window
{
public:
	explicit Output(HWND hParentWindow);

	void Clear(void);
	void WriteLine(const wchar_t* lpszFormat, ...);
	void Write(const wchar_t* lpszFormat, ...);
};

