#pragma once

#include "Window.h"
#include "SourceTab.h"

#include <vector>

typedef std::vector<SourceTab*> TabList;

class WorkArea : public Window
{
private:
	HRESULT InitializeSourceEditorWindow(HINSTANCE hInstance);
	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnCloseTab(HWND hWnd, LPARAM lParam);
	LRESULT OnPaint(HWND hWnd);

	HFONT hBkFont = nullptr;

	int m_SourceIndex = 0;
	TabList m_Tabs;
	TabList m_ClosedTabs;

	void UpdateBackgroundFont(void);
	void InsertSourceTab(SourceTab* pSourceTab);
	void CreateTab(wchar_t* lpszFileName);

	int GetSelectedTabIndex(void) const;

public:
	explicit WorkArea(HWND hParentWindow);
	~WorkArea(void);

	void OnDPIChanged(void);
	void UnselectAllTabs(void);
	void SelectFileFromName(wchar_t* lpszName);
	void CloseAllTabs(void);

	TabList& GetVisibleTabs(void);
	TabList& GetHiddenTabs(void);

	SourceTab* GetSelectedTab(void);

	LRESULT WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

