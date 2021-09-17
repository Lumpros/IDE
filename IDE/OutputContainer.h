#pragma once

#include "Window.h"
#include "ODButton.h"
#include "Output.h"
#include "Explorer.h"
#include "StatusBar.h"
#include "WorkArea.h"

class OutputContainer : public Window
{
private:
	HRESULT InitializeOutputWindow(HINSTANCE hInstance);

	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnNCHitTest(HWND hWnd);
	LRESULT OnWindowPosChanged(HWND hWnd, LPARAM lParam);
	LRESULT OnPaint(HWND hWnd);
	LRESULT OnGetMinMaxInfo(HWND hWnd, LPARAM lParam);
	LRESULT OnCommand(HWND hWnd, WPARAM wParam);
	LRESULT OnDrawItem(HWND hWnd, LPARAM lParam);
	LRESULT OnNotify(HWND hWnd, LPARAM lParam);

	void SetButtonPositions(void);
	void RestrictButtonSectorSize(LPWINDOWPOS pWindowPos, const RECT& rcParent, StatusBar* pStatusBar);
	void ResizeWorkArea(WorkArea* pWorkArea, Explorer* pExplorer, LPWINDOWPOS pWindowPos, const RECT& rcParent);

	int GetMinimumHeight(void);

	ODButton* m_pOutputButton = nullptr;
	ODButton* m_pErrorListButton = nullptr;
	Output* m_pOutput = nullptr;

public:
	explicit OutputContainer(HWND hWnd);
	~OutputContainer(void);

	LRESULT WindowProcedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

