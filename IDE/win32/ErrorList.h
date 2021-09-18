#pragma once

#include "Window.h"

class ErrorList : public Window
{
private:
	void InitializeListViewColumns(void);
	void UpdateListViewColumnWidths(void);

public:
	explicit ErrorList(HWND hParentWindow);

	void OnDPIChange(void);
};

