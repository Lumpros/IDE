#include "WorkArea.h"
#include "Logger.h"
#include "Utility.h"
#include "AppWindow.h"

#include <Richedit.h>

#define SOURCE_EDITOR_CLASS L"IDEClass"

#define NO_TABS_AVAILABLE (-1)

static HRESULT RegisterSourceEditorWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK SourceEditorWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

constexpr int iTabHeight = 20;

WorkArea::WorkArea(HWND hParentWindow)
{
	const HINSTANCE hInstance = GetModuleHandle(nullptr);

	this->m_hWndParent = hParentWindow;

	if (FAILED(RegisterSourceEditorWindowClass(hInstance)))
	{
		Logger::Write(L"Failed to register source editor window class!");
		PostQuitMessage(1);
		return;
	}

	if (FAILED(InitializeSourceEditorWindow(hInstance)))
	{
		Logger::Write(L"Failed to initialize source editor window!");
		PostQuitMessage(1);
		return;
	}
}

WorkArea::~WorkArea(void)
{
	for (SourceTab* pSourceTab : m_Tabs) {
		delete pSourceTab;
	}

	m_Tabs.clear();

	SAFE_DELETE_GDIOBJ(hBkFont);
}

static HRESULT RegisterSourceEditorWindowClass(HINSTANCE hInstance)
{
	static bool hasBeenRegistered = false;

	if (!hasBeenRegistered)
	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(WNDCLASSEX));
		wcex.cbSize = sizeof(wcex);
		wcex.lpszClassName = SOURCE_EDITOR_CLASS;
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH));
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpfnWndProc = ::SourceEditorWindowProcedure;
		wcex.cbWndExtra = sizeof(WorkArea*);
		wcex.style = CS_HREDRAW | CS_VREDRAW;

		if (!RegisterClassEx(&wcex))
			return E_FAIL;

		hasBeenRegistered = true;
	}

	return S_OK;
}

HRESULT WorkArea::InitializeSourceEditorWindow(HINSTANCE hInstance)
{
	m_hWndSelf = CreateWindowEx(
		0,
		SOURCE_EDITOR_CLASS,
		nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0, 0, 0,
		m_hWndParent,
		nullptr,
		hInstance,
		this
	);

	if (!m_hWndSelf) {
		return E_FAIL;
	}

	this->UpdateBackgroundFont();

	return S_OK;
}

void WorkArea::UpdateBackgroundFont(void)
{
	SAFE_DELETE_GDIOBJ(hBkFont);

	int iFontHeight = static_cast<int>(18 * Utility::GetScaleForDPI(m_hWndSelf));

	hBkFont = CreateFont(
		iFontHeight,
		0, 0, 0,
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		0, 0, 0, 
		0,
		0,
		L"Segoe UI"
	);
}

void WorkArea::InsertSourceTab(SourceTab* pSourceTab)
{
	int iTabX = 0;

	for (SourceTab* pTab : m_Tabs) 
	{
		iTabX += pTab->GetRect().right;
	}

	pSourceTab->SetPos(iTabX, 0);
	pSourceTab->SetSize(
		pSourceTab->GetRequiredTabWidth(),
		static_cast<int>(iTabHeight * Utility::GetScaleForDPI(m_hWndParent)));

	m_Tabs.push_back(pSourceTab);
}

void WorkArea::OnDPIChanged(void)
{
	if (!m_Tabs.empty())
	{
		int iHeight = static_cast<int>(iTabHeight * Utility::GetScaleForDPI(m_hWndParent));

		int iTabX = 0;

		for (size_t i = 0; i < m_Tabs.size(); ++i)
		{
			SourceEdit* pSourceEdit = m_Tabs[i]->GetSourceEdit();
			pSourceEdit->AdjustLeftMarginForDPI();
			pSourceEdit->AdjustFontForDPI();

			int iTabWidth = m_Tabs[i]->GetRequiredTabWidth();

			m_Tabs[i]->SetPos(iTabX, 0);
			m_Tabs[i]->SetSize(iTabWidth, iHeight);

			if (m_Tabs[i]->IsSelected())
			{
				SetWindowPos(
					pSourceEdit->GetHandle(),
					nullptr,
					0,
					m_Tabs[i]->GetRect().bottom + 1,
					m_rcSelf.right,
					m_rcSelf.bottom - 1,
					SWP_NOZORDER
				);
			}

			iTabX += iTabWidth;
		}
	}

	this->UpdateBackgroundFont();
}

void WorkArea::UnselectAllTabs(void)
{
	for (size_t i = 0; i < m_Tabs.size(); ++i)
	{
		m_Tabs[i]->Unselect();
	}
}

LRESULT WorkArea::WindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_PAINT:
		return OnPaint(hWnd);

	case WM_SIZE:
		return OnSize(hWnd, lParam);

	case WM_CLOSE_TAB:
		return OnCloseTab(hWnd, lParam);

	case WM_TAB_SELECTED:
	{
		SourceTab* pSourceTab = reinterpret_cast<SourceTab*>(lParam);

		for (size_t i = 0; i < m_Tabs.size(); ++i)
		{
			if (m_Tabs[i] == pSourceTab)
			{
				m_SourceIndex = i;
				break;
			}
		}
	}
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WorkArea::OnPaint(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC = BeginPaint(hWnd, &ps);

	if (m_Tabs.empty())
	{
		SelectObject(hDC, hBkFont);
		SetBkMode(hDC, TRANSPARENT);
		m_rcSelf.left = 0;
		FillRect(hDC, &m_rcSelf, (HBRUSH)GetStockObject(LTGRAY_BRUSH));

		Utility::DrawTextCentered(hDC, m_rcSelf, L"Opened files are displayed here");
	}

	else
	{
		RECT rcNew = m_rcSelf;
		rcNew.left = 0;
		rcNew.bottom = m_Tabs.front()->GetRect().bottom + 1;

		FillRect(hDC, &rcNew, (HBRUSH)GetStockObject(LTGRAY_BRUSH));
	}

	EndPaint(hWnd, &ps);
	return 0;
}

LRESULT WorkArea::OnSize(HWND hWnd, LPARAM lParam)
{
	m_rcSelf.right = LOWORD(lParam);
	m_rcSelf.bottom = HIWORD(lParam);

	const int starting_y = static_cast<int>(iTabHeight * Utility::GetScaleForDPI(m_hWndParent)) + 1;

	m_SourceIndex = GetSelectedTabIndex();

	if (m_Tabs.size() > static_cast<size_t>(m_SourceIndex))
	{
		AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(m_hWndParent);

		SetWindowPos(
			m_Tabs[m_SourceIndex]->GetSourceEdit()->GetHandle(),
			nullptr,
			0,
			starting_y,
			m_rcSelf.right,
			m_rcSelf.bottom - starting_y,
			SWP_NOZORDER
		);
	}
	
	return 0;
}

LRESULT WorkArea::OnCloseTab(HWND hWnd, LPARAM lParam)
{
	SourceTab* pSourceTab = reinterpret_cast<SourceTab*>(lParam);
	pSourceTab->Hide();

	const bool isSelected = pSourceTab->IsSelected();

	if (isSelected)
	{
		pSourceTab->Unselect();
		pSourceTab->GetSourceEdit()->Hide();
	}

	if (m_Tabs.size() == 1) 
	{
		StatusBar* pStatusBar = GetAssociatedObject<AppWindow>(m_hWndParent)->GetStatusBar();

		if (pStatusBar) {
			pStatusBar->ClearEditorInformation();
		}
	}

	size_t index = 0;
	size_t iTabWidth = static_cast<size_t>(80 * Utility::GetScaleForDPI(m_hWndParent));

	for (size_t i = 0; i < m_Tabs.size(); ++i)
	{
		if (m_Tabs[i] == pSourceTab)
		{
			if (m_Tabs.size() != 1 && isSelected)
			{
				// Try to select the tab at the right of the selected one
				try {
					m_Tabs.at(i + 1)->Select();
					m_SourceIndex = i + 1;
				// If an exception is thrown, there wont be one to the right
				// So select the one to the left
				// We know there will be one to the left, since size > 1 and
				// There arent any tabs to the left (=> there has to be one to the left)
				} catch (...) {
					m_Tabs.at(i - 1)->Select();
					m_SourceIndex = i - 1;
				}
			}
			
			// Move the tab to the closed tabs
			if (!m_Tabs[i]->IsTemporary()) {
				m_ClosedTabs.push_back(m_Tabs[i]);
			}

			else {
				DestroyWindow(m_Tabs[i]->GetHandle());
				delete m_Tabs[i];
			}

			m_Tabs.erase(m_Tabs.begin() + i);

			break;
		}
	}

	// Place the tabs to the correct places after closing one
	for (size_t index = 0, x_pos = 0; index < m_Tabs.size(); ++index)
	{
		m_Tabs[index]->SetPos(x_pos, 0);

		x_pos += m_Tabs[index]->GetRect().right;
	}

	return 0;
}

// Compare the tabs' full paths so there aren't any mix ups
// For example: directory1/main.cpp and directory2/main.cpp
// Have to open different tabs
void WorkArea::SelectFileFromName(wchar_t* lpszName)
{
	m_SourceIndex = this->GetSelectedTabIndex();

	/* Look through opened tabs */
	/* If one of them has the same name, select it */
	for (size_t i = 0; i < m_Tabs.size(); ++i)
	{
		LPCWSTR lpName = m_Tabs[i]->GetPath();

		if (lstrcmp(lpName, lpszName) == 0)
		{
			m_Tabs[m_SourceIndex]->Unselect();
			m_Tabs[i]->Select();
			m_SourceIndex = i;
			return;
		}
	}

	const bool areTabsOpened = (m_Tabs.size() > 0);

	/* Look through closed tabs*/
	/* If it's found, open it and select it*/
	for (size_t i = 0; i < m_ClosedTabs.size(); ++i)
	{
		LPCWSTR lpName = m_ClosedTabs[i]->GetPath();

		if (lstrcmp(lpName, lpszName) == 0)
		{
			if (areTabsOpened && m_SourceIndex < (int)m_Tabs.size())
			{
				m_Tabs[m_SourceIndex]->Unselect();
			}

			m_Tabs.push_back(m_ClosedTabs[i]);
			m_SourceIndex = m_Tabs.size() - 1;
			m_ClosedTabs[i]->Show();
			m_ClosedTabs[i]->Select();

			int x = 0;

			for (int k = 0; k < (int)m_Tabs.size(); ++k)
			{
				if (m_Tabs[k] != m_ClosedTabs[i])
				{
					x += m_Tabs[k]->GetRect().right;
				}
			}

			m_ClosedTabs[i]->HideCloseButton();
			m_ClosedTabs[i]->SetPos(x, 0);
			m_ClosedTabs.erase(m_ClosedTabs.begin() + i);
			return;
		}
	}

	/* if it was neither open nor closed, create it and select it*/
	CreateTab(lpszName);
}

TabList& WorkArea::GetVisibleTabs(void)
{
	return m_Tabs;
}

TabList& WorkArea::GetHiddenTabs(void)
{
	return m_ClosedTabs;
}

void WorkArea::CloseAllTabs(void)
{
	for (SourceTab* pSourceTab : m_Tabs)
	{
		if (pSourceTab->IsSelected())
		{
			pSourceTab->Unselect();
		}

		pSourceTab->Hide();

		delete pSourceTab;
	}

	m_Tabs.clear();

	for (SourceTab* pSourceTab : m_ClosedTabs)
	{
		delete pSourceTab;
	}

	m_ClosedTabs.clear();

	m_SourceIndex = NO_TABS_AVAILABLE;

	StatusBar* pStatusBar = GetAssociatedObject<AppWindow>(m_hWndParent)->GetStatusBar();

	if (pStatusBar) {
		pStatusBar->ClearEditorInformation();
	}
}

SourceTab* WorkArea::CreateTab(wchar_t* lpszFileName)
{
	if (m_SourceIndex != NO_TABS_AVAILABLE && m_SourceIndex < (int)m_Tabs.size())
	{
		m_Tabs[m_SourceIndex]->Unselect();
	}

	SourceTab* pSourceTab = new SourceTab(m_hWndSelf);
	pSourceTab->SetName(lpszFileName);
	pSourceTab->SetEditTextToContentsOfFile(lpszFileName);
	InsertSourceTab(pSourceTab);

	m_Tabs.back()->Select();
	m_SourceIndex = m_Tabs.size() - 1;

	return pSourceTab;
}

SourceTab* WorkArea::CreateTemporaryTab(wchar_t* lpszFileName)
{
	SourceTab* pTemporaryTab = this->CreateTab(lpszFileName);

	pTemporaryTab->SetTemporary(true);

	return pTemporaryTab;
}

int WorkArea::GetSelectedTabIndex(void) const
{
	for (size_t i = 0; i < m_Tabs.size(); ++i)
	{
		if (m_Tabs[i]->IsSelected())
		{
			return i;
		}
	}

	return NO_TABS_AVAILABLE;
}

SourceTab* WorkArea::GetSelectedTab(void)
{
	m_SourceIndex = this->GetSelectedTabIndex();

	if (m_SourceIndex != NO_TABS_AVAILABLE && !m_Tabs.empty())
	{
		try {
			return m_Tabs.at(m_SourceIndex);
		} catch (...) {
			return nullptr;
		}
	}

	return nullptr;
}

static LRESULT OnCreate(HWND hWnd, LPARAM lParam)
{
	SetWindowLongPtr(
		hWnd, GWLP_USERDATA,
		reinterpret_cast<LONG>(reinterpret_cast<LPCREATESTRUCT>(lParam)->lpCreateParams)
	);

	return 0;
}

static LRESULT CALLBACK SourceEditorWindowProcedure(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return OnCreate(hWnd, lParam);

	case WM_ERASEBKGND:
		return FALSE;

	default:
		WorkArea* pSourceEditor = GetAssociatedObject<WorkArea>(hWnd);

		if (pSourceEditor)
		{
			return pSourceEditor->WindowProcedure(hWnd, uMsg, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}