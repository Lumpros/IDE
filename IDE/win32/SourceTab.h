#pragma once

#include "Window.h"
#include "SourceEdit.h"

// lParam: Pointer to the SourceTab oobject to be closed
#define WM_CLOSE_TAB (WM_APP + 1) 

// LPARAM = pointer to SourceTab
#define WM_TAB_SELECTED (WM_APP + 2)

struct SourceInfo {
	SourceEdit* m_pSourceEdit = nullptr;
	wchar_t* lpszFileName = nullptr;
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
	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnCommand(HWND hWnd, WPARAM wParam);

	COLORREF crButton = RGB(0xFF, 0, 0);
	SourceInfo m_sInfo;
	HWND m_hCloseButton = nullptr;
	HWND m_hAbsolutePathTooltip = nullptr;
	HWND m_hTooltip = nullptr;
	bool m_IsSelected = false;
	bool m_IsTrackingMouse = false;
	bool m_IsTemporary = false;

public:
	explicit SourceTab(HWND hParentWindow);
	~SourceTab(void);

	SourceEdit* GetSourceEdit(void) const { if (m_sInfo.m_pSourceEdit) return m_sInfo.m_pSourceEdit; else return nullptr; }

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	bool IsSelected(void);
	void Select(void);
	void Unselect(void);
	void SetName(LPCWSTR lpszName);
	void HideCloseButton(void) const;
	void SetEditTextToContentsOfFile(LPCWSTR lpPath);
	void RemoveAsteriskFromDisplayedName(void);
	void SetTemporary(bool temporary);
	int GetRequiredTabWidth(void) const;
	bool IsTemporary(void) const;

	const wchar_t* GetPath(void) const {
		return m_sInfo.lpszFileName;
	}

	/* Must be deleted by the caller */
	const wchar_t* GetName(void) const { 
		int nMaxCount = GetWindowTextLength(m_hWndSelf) + 1;
		LPWSTR lpName = new wchar_t[nMaxCount];
		GetWindowText(m_hWndSelf, lpName, nMaxCount);
		return lpName;
	}
};

