#pragma once

#include "Window.h"
#include "Explorer.h"
#include "WorkArea.h"
#include "Output.h"

class AppWindow : public Window
{
private:
	HRESULT InitializeWindow(HINSTANCE hInstance);
	Explorer* m_pExplorer = nullptr;
	WorkArea* m_pWorkArea = nullptr;
	Output* m_pOutput = nullptr;

	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnGetMinMax(HWND hWnd, LPARAM lParam);
	LRESULT OnDPIChanged(HWND hWnd, LPARAM lParam);

	HRESULT InitializeComponents(void);

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

	inline Output* GetOutputWindow(void) const {
		return m_pOutput;
	}
};

