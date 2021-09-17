#pragma once

#include "Window.h"

class Output : public Window
{
private:
	void InitializeListViewColumns(void);
	void UpdateListViewColumnWidths(void);

public:
	explicit Output(HWND hParentWindow);

	void OnDPIChange(void);
};

