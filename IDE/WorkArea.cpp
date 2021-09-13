#include "WorkArea.h"
#include "Logger.h"
#include "Utility.h"

#include <Richedit.h>

#define SOURCE_EDITOR_CLASS L"IDESourceEditorClass"

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
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(GRAY_BRUSH));
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.lpfnWndProc = ::SourceEditorWindowProcedure;
		wcex.cbWndExtra = sizeof(WorkArea*);

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
		WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
		0, 0, 0, 0,
		m_hWndParent,
		nullptr,
		hInstance,
		this
	);

	if (!m_hWndSelf) {
		return E_FAIL;
	}


	SourceTab* pSourceTab = new SourceTab(m_hWndSelf);
	pSourceTab->SetName(L"main.c");
	pSourceTab->Select();
	InsertSourceTab(pSourceTab);

	pSourceTab = new SourceTab(m_hWndSelf);
	pSourceTab->SetName(L"app.h");
	InsertSourceTab(pSourceTab);

	pSourceTab = new SourceTab(m_hWndSelf);
	pSourceTab->SetName(L"app.c");
	InsertSourceTab(pSourceTab);

	return S_OK;
}

void WorkArea::InsertSourceTab(SourceTab* pSourceTab)
{
	int iTabWidth = static_cast<int>(80 * Utility::GetScaleForDPI(m_hWndParent));

	const int iTabX = iTabWidth * m_Tabs.size();

	pSourceTab->SetPos(iTabX, 0);
	pSourceTab->SetSize(iTabWidth, (int)(iTabHeight * Utility::GetScaleForDPI(m_hWndParent)));

	m_Tabs.push_back(pSourceTab);
}

void WorkArea::OnDPIChanged(void)
{
	int iTabWidth = static_cast<int>(80 * Utility::GetScaleForDPI(m_hWndParent));
	int iHeight = static_cast<int>(iTabHeight * Utility::GetScaleForDPI(m_hWndParent));

	for (size_t i = 0; i < m_Tabs.size(); ++i)
	{
		const int iTabX = iTabWidth * i;
		
		SourceEdit* pSourceEdit = m_Tabs[i]->GetSourceEdit();
		pSourceEdit->AdjustLeftMarginForDPI();
		pSourceEdit->AdjustFontForDPI();

		m_Tabs[i]->SetPos(iTabX, 0);
		m_Tabs[i]->SetSize(iTabWidth, iHeight);

		if (m_Tabs[i]->IsSelected()) 
		{
			SetWindowPos(
				pSourceEdit->GetHandle(),
				nullptr,
				0,
				m_Tabs[i]->GetRect().bottom,
				m_rcSelf.right,
				m_rcSelf.bottom,
				SWP_NOZORDER
			);
		}
	}
	
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
	case WM_SIZE:
		return OnSize(hWnd, lParam);

	case WM_CLOSE_TAB:
		return OnCloseTab(hWnd, lParam);
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT WorkArea::OnSize(HWND hWnd, LPARAM lParam)
{
	m_rcSelf.right = LOWORD(lParam);
	m_rcSelf.bottom = HIWORD(lParam);

	const int starting_y = static_cast<int>(iTabHeight * Utility::GetScaleForDPI(m_hWndParent));

	m_SourceIndex = GetSelectedTabIndex();

	if (m_Tabs.size() > m_SourceIndex)
	{
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

	size_t index = 0;
	size_t iTabWidth = static_cast<size_t>(80 * Utility::GetScaleForDPI(m_hWndParent));

	for (size_t i = 0; i < m_Tabs.size(); ++i)
	{
		if (m_Tabs[i] == pSourceTab)
		{
			if (m_Tabs.size() != 1 && isSelected)
			{
				try {
					m_Tabs.at(i + 1)->Select();
					m_SourceIndex = i + 1;
				} catch (...) {
					m_Tabs.at(i - 1)->Select();
					m_SourceIndex = i - 1;
				}
			}

			m_ClosedTabs.push_back(m_Tabs[i]);
			m_Tabs[i]->HideCloseButton();
			m_Tabs.erase(m_Tabs.begin() + i);
		}
	}

	for (size_t index = 0; index < m_Tabs.size(); ++index)
	{
		m_Tabs[index]->SetPos(iTabWidth * index, 0);
	}

	return 0;
}

void WorkArea::SelectFileFromName(wchar_t* lpszName)
{
	for (size_t i = 0; i < m_Tabs.size(); ++i)
	{
		if (lstrcmp(m_Tabs[i]->GetName(), lpszName) == 0)
		{
			m_Tabs[m_SourceIndex]->Unselect();
			m_Tabs[i]->Select();
			m_SourceIndex = i;
			return;
		}
	}

	const bool areTabsOpened = (m_Tabs.size() > 0);

	for (size_t i = 0; i < m_ClosedTabs.size(); ++i)
	{
		if (lstrcmp(m_ClosedTabs[i]->GetName(), lpszName) == 0)
		{
			if (areTabsOpened)
			{
				m_Tabs[m_SourceIndex]->Unselect();
			}

			m_Tabs.push_back(m_ClosedTabs[i]);
			m_SourceIndex = m_Tabs.size() - 1;
			m_ClosedTabs[i]->Show();
			m_ClosedTabs[i]->Select();
			m_ClosedTabs[i]->SetPos(m_ClosedTabs[i]->GetRect().right * m_SourceIndex, 0);
			m_ClosedTabs.erase(m_ClosedTabs.begin() + i);
			break;
		}
	}
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

	return -1;
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

	default:
		WorkArea* pSourceEditor = GetAssociatedObject<WorkArea>(hWnd);

		if (pSourceEditor)
		{
			return pSourceEditor->WindowProcedure(hWnd, uMsg, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}