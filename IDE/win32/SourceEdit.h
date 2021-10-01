#pragma once

#include "Window.h"
#include "StatusBar.h"
#include "Zoomer.h"

class SourceEdit : public Window
{
private:
	int GetLeftMargin(void) const;

	HFONT m_hFont = nullptr;
	StatusBar* m_pStatusBar = nullptr;
	Zoomer m_Zoomer;
	bool m_haveContentsBeenEdited = false;

	void SetLineColumnStatusBar(void);

public:
	explicit SourceEdit(HWND hParentWindow);
	~SourceEdit(void);

	inline void ZoomIn(void) { m_Zoomer.ZoomIn(); }
	inline void ZoomOut(void) { m_Zoomer.ZoomOut(); }
	inline void RestoreZoom(void) { m_Zoomer.RestoreZoom(); }

	void AdjustLeftMarginForDPI(void);
	void AdjustFontForDPI(void);

	void MarkAsEdited(void);
	void MarkAsUnedited(void);

	void RefreshStatusBarText(void);
	void HandleMouseWheel(WPARAM wParam);

	bool HasBeenEdited(void) const { return m_haveContentsBeenEdited; };
};

