#pragma once

#include "Window.h"
#include "Explorer.h"
#include "WorkArea.h"

class AppWindow : public Window
{
private:
	HRESULT InitializeWindow(HINSTANCE hInstance);
	Explorer* m_pExplorer = nullptr;
	WorkArea* m_pWorkArea = nullptr;

	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnGetMinMax(HWND hWnd, LPARAM lParam);
	LRESULT OnDPIChanged(HWND hWnd, LPARAM lParam);

public:
	~AppWindow(void);

	void Initialize(HINSTANCE hInstance);

	LRESULT WindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	inline WorkArea* GetWorkArea(void) const {
		return m_pWorkArea;
	}

	inline Explorer* GetExplorer(void) const {
		return m_pExplorer;
	}
};

