#pragma once

#include "Window.h"
#include "SourceEdit.h"

// lParam: Pointer to the SourceTab oobject to be closed
#define WM_CLOSE_TAB (WM_APP + 1)

struct SourceInfo {
	SourceEdit* m_pSourceEdit = nullptr;
	const wchar_t* lpszFileName = nullptr;
};

class SourceTab : public Window
{
private:
	HRESULT InitializeSourceTabWindow(HINSTANCE hInstance);
	LRESULT OnEraseBackground(HWND hWnd, WPARAM wParam);
	LRESULT OnPaint(HWND hWnd);
	LRESULT OnLButtonDown(HWND hWnd);
	LRESULT OnMouseMove(HWND hWnd);
	LRESULT OnMouseLeave(HWND hWnd);
	LRESULT OnDrawItem(HWND hWnd, LPARAM lParam);
	LRESULT OnSize(HWND hWnd);
	LRESULT OnCommand(HWND hWnd, WPARAM wParam);

	SourceInfo m_sInfo;
	HWND m_hCloseButton = nullptr;
	bool m_IsSelected = false;
	bool m_IsTrackingMouse = false;

public:
	explicit SourceTab(HWND hParentWindow);
	~SourceTab(void);

	SourceEdit* GetSourceEdit(void) const { if (m_sInfo.m_pSourceEdit) return m_sInfo.m_pSourceEdit; else return nullptr; }

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool IsSelected(void);
	void Select(void);
	void Unselect(void);
	void SetName(LPCWSTR lpszName);
};

