#pragma once

#include "StatusBar.h"

class SourceEdit;

/// <summary>
/// All the functions was taken from my notepad project
/// but instead of C functions i wrapped it in a class
/// </summary>
class Zoomer
{
public:
	Zoomer(SourceEdit* pSourceEdit);

	void AttachStatusBar(StatusBar* pStatusBar);
	void SetStatusBarZoomText(void);
	void Update(short bWheelDelta);
	void ZoomIn(void);
	void ZoomOut(void);
	void RestoreZoom(void);
	int GetZoomLevel(void) const;

private:
	void RestrictNumeratorRange(void);
	void RequestZoom(void);
	int RoundValueToTen(int value, short bWheelData) const;

private:
	StatusBar* m_pStatusBar = nullptr;
	SourceEdit* m_pSourceEdit = nullptr;

	int m_iZoomDenominator = 90;
	int m_iZoomNumerator = 100;
};

