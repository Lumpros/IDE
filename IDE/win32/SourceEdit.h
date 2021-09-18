#pragma once

#include "Window.h"

class SourceEdit : public Window
{
private:
	int GetLeftMargin(void) const;

	HFONT m_hFont = nullptr;

public:
	explicit SourceEdit(HWND hParentWindow);
	~SourceEdit(void);

	void AdjustLeftMarginForDPI(void);
	void AdjustFontForDPI(void);
};

