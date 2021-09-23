#pragma once

#include "Window.h"

class SourceEdit : public Window
{
private:
	int GetLeftMargin(void) const;

	HFONT m_hFont = nullptr;

	bool m_haveContentsBeenEdited;

public:
	explicit SourceEdit(HWND hParentWindow);
	~SourceEdit(void);

	void AdjustLeftMarginForDPI(void);
	void AdjustFontForDPI(void);

	void MarkAsEdited(void);
	void MarkAsUnedited(void);

	bool HasBeenEdited(void) const { return m_haveContentsBeenEdited; };
};

