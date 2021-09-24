#pragma once

#include "Window.h"
#include "SourceTab.h"

#include <vector>

typedef std::vector<SourceTab*> TabList;

class WorkArea : public Window
{
public:
	explicit WorkArea(HWND hParentWindow);
	~WorkArea(void);

	void OnDPIChanged(void);
	void UnselectAllTabs(void);
	void SelectFileFromName(wchar_t* lpszName);
	void CloseAllTabs(void);
	SourceTab* CreateTab(wchar_t* lpszFileName);

	/// <summary>
	/// A temporary tab is deleted after it has been closed
	/// </summary>
	/// <param name="lpszFileName"> Absolute path of file </param>
	SourceTab* CreateTemporaryTab(wchar_t* lpszFileName);

	TabList& GetVisibleTabs(void);
	TabList& GetHiddenTabs(void);

	SourceTab* GetSelectedTab(void);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	HRESULT InitializeSourceEditorWindow(HINSTANCE hInstance);
	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnCloseTab(HWND hWnd, LPARAM lParam);
	LRESULT OnPaint(HWND hWnd);

	void UpdateBackgroundFont(void);
	void InsertSourceTab(SourceTab* pSourceTab);
	int GetSelectedTabIndex(void) const;

private:
	HFONT hBkFont = nullptr;
	int m_SourceIndex = 0;
	TabList m_Tabs;
	TabList m_ClosedTabs;
};

