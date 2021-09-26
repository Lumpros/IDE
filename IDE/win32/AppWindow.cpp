#include "AppWindow.h"
#include "Utility.h"
#include "Logger.h"
#include "resource.h"
#include "ColorFormatParser.h"
#include "Wordifier.h"

#include <CommCtrl.h>
#include <ShlObj_core.h>
#include <Shlwapi.h>
#include <commdlg.h>

#pragma comment(lib, "Shlwapi.lib")

#define APP_WINDOW_CLASS L"IDEAppWindow"

static HRESULT RegisterAppWindowClass(HINSTANCE hInstance);
static LRESULT CALLBACK AppWindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

AppWindow::~AppWindow(void)
{
	SAFE_DELETE_PTR(m_pExplorer);
	SAFE_DELETE_PTR(m_pWorkArea);
	SAFE_DELETE_PTR(m_pOutputContainer);
	SAFE_DELETE_PTR(m_pStatusBar);
}

static inline bool HasArguements(LPWSTR lpCmdLine)
{
	return lstrlen(lpCmdLine) > 0;
}

void AppWindow::Initialize(HINSTANCE hInstance, LPWSTR lpCmdLine)
{
	if (FAILED(::RegisterAppWindowClass(hInstance)))
	{
		Logger::Write(L"Failed to register app window class!");
		Logger::CloseOutputFile();
		ExitProcess(1);
	}

	if (FAILED(InitializeWindow(hInstance)))
	{
		Logger::Write(L"Window initialization failed!");
		Logger::CloseOutputFile();
		ExitProcess(1);
	}

	if (HasArguements(lpCmdLine))
	{
		OpenFolderFromCommandLine(lpCmdLine);
	}
}

void AppWindow::OpenFolderFromCommandLine(LPWSTR lpCmdLine)
{
	wchar_t absolute_path[MAX_PATH];

	if (lpCmdLine[0] == L'\"')
	{
		lpCmdLine[lstrlen(lpCmdLine) - 1] = L'\0';
		++lpCmdLine;
	}

	wchar_t* pPath = lpCmdLine;

	if (PathIsRelative(lpCmdLine))
	{
		pPath = _wfullpath(absolute_path, lpCmdLine, MAX_PATH);
	}

	if (pPath != nullptr)
	{
		if (Utility::IsPathDirectory(lpCmdLine))
		{
			m_pStatusBar->SetText(L"Opening folder...", 0);
			m_pExplorer->OpenProjectFolder(lpCmdLine);
			m_pStatusBar->SetText(L"Folder opened", 0);
		}

		else
		{
			m_pStatusBar->SetText(L"Invalid directory entered.", 0);
		}
	}

	else
	{
		m_pStatusBar->SetText(L"Failed to open folder.", 0);
	}
}

static HRESULT RegisterAppWindowClass(HINSTANCE hInstance)
{
	static bool hasBeenRegistered = false;

	if (!hasBeenRegistered)
	{
		WNDCLASSEX wcex;
		ZeroMemory(&wcex, sizeof(WNDCLASSEX));
		wcex.cbSize = sizeof(wcex);
		wcex.lpszClassName = APP_WINDOW_CLASS;
		wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW);
		wcex.hInstance = hInstance;
		wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		wcex.lpfnWndProc = ::AppWindowProcedure;
		wcex.cbWndExtra = sizeof(AppWindow*);

		if (!RegisterClassEx(&wcex))
			return E_FAIL;
		
		hasBeenRegistered = true;
	}

	return S_OK;
}

HRESULT AppWindow::InitializeWindow(HINSTANCE hInstance)
{
	m_hWndSelf = CreateWindowEx(
		NULL,
		APP_WINDOW_CLASS,
		L"IDE",
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1200, 800,
		nullptr,
		nullptr,
		hInstance,
		this
	);

	if (m_hWndSelf == nullptr)
	{
		return E_FAIL;
	}

	SetMenu(m_hWndSelf,
		LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MENU1))
	);

	RECT rcClient;
	GetClientRect(m_hWndSelf, &rcClient);
	m_rcSelf.right = rcClient.right;
	m_rcSelf.bottom = rcClient.bottom;

	if (FAILED(InitializeComponents()))
	{
		Logger::Write(L"Failed to initialize one or more components");
		return E_FAIL;
	}

	Utility::CenterWindowRelativeToParent(m_hWndSelf);

	m_pStatusBar->SetText(L"Initialized", 0);

	ShowWindow(m_hWndSelf, SW_SHOWMAXIMIZED);
	UpdateWindow(m_hWndSelf);

	return S_OK;
}

HRESULT AppWindow::InitializeComponents(void)
{
	HRESULT hr = E_FAIL;

	m_pStatusBar = new StatusBar(m_hWndSelf);
	m_pWorkArea = new WorkArea(m_hWndSelf);

	if (m_pWorkArea && m_pStatusBar)
	{
		m_pExplorer = new Explorer(m_hWndSelf);
		m_pExplorer->SetStatusBar(m_pStatusBar);

		if (m_pExplorer)
		{
			m_pExplorer->SetSize(static_cast<int>(250 * Utility::GetScaleForDPI(m_hWndSelf)), 0);

			m_pOutputContainer = new OutputContainer(m_hWndSelf);

			if (m_pOutputContainer)
			{
				RECT rcStatusBar;
				GetClientRect(m_pStatusBar->GetHandle(), &rcStatusBar);

				m_pOutputContainer->SetPos(
					m_pExplorer->GetRect().right + 3,
					m_rcSelf.bottom * 3 / 4 + 3 - rcStatusBar.bottom
				);

				m_pOutputContainer->SetSize(
					m_rcSelf.right - m_pExplorer->GetRect().right - 6,
					m_rcSelf.bottom / 4 - 3 - rcStatusBar.bottom
				);

				hr = S_OK;
			}
		}
	}

	return hr;
}

LRESULT AppWindow::WindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_COMMAND:
		return OnCommand(hWnd, wParam);

	case WM_SIZE:
		return OnSize(hWnd, lParam);

	case WM_GETMINMAXINFO:
		return OnGetMinMax(hWnd, lParam);

	case WM_DPICHANGED:
		return OnDPIChanged(hWnd, lParam);
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}

LRESULT AppWindow::OnSize(HWND hWnd, LPARAM lParam)
{
	m_rcSelf.right = LOWORD(lParam);
	m_rcSelf.bottom = HIWORD(lParam);

	if (m_pStatusBar)
	{
		SendMessage(m_pStatusBar->GetHandle(), WM_SIZE, 0, 0);

		RECT rcStatusBar;
		GetClientRect(m_pStatusBar->GetHandle(), &rcStatusBar);

		const int iStatusBarHeight = rcStatusBar.bottom;

		if (m_pExplorer)
		{
			int iExplorerWidth;

			if (IsIconic(hWnd))
			{
				iExplorerWidth = m_pExplorer->GetRect().right;
			}

			else
			{
				iExplorerWidth = min(m_pExplorer->GetRect().right, m_rcSelf.right);
			}

			const int iWorkAreaHeight = m_pWorkArea->GetRect().bottom;

			m_pExplorer->SetSize(iExplorerWidth, m_rcSelf.bottom - iStatusBarHeight);

			if (m_pWorkArea)
			{
				m_pWorkArea->SetPos(iExplorerWidth, 0);

				if (m_pOutputContainer)
				{
					m_pOutputContainer->SetPos(
						iExplorerWidth,
						m_rcSelf.bottom - m_pOutputContainer->GetRect().bottom - iStatusBarHeight
					);

					m_pOutputContainer->SetSize(
						m_rcSelf.right - iExplorerWidth,
						m_pOutputContainer->GetRect().bottom
					);
				}
			}
		}
	}

	return 0;
}
 
LRESULT AppWindow::OnGetMinMax(HWND hWnd, LPARAM lParam)
{
	LPMINMAXINFO mmi = reinterpret_cast<LPMINMAXINFO>(lParam);

	const int size = static_cast<int>(Utility::GetScaleForDPI(hWnd) * 240);

	mmi->ptMinTrackSize = POINT { size, size };

	return 0;
}

LRESULT AppWindow::OnDPIChanged(HWND hWnd, LPARAM lParam)
{
	LPRECT pRect = reinterpret_cast<LPRECT>(lParam);

	Utility::UpdateFont(hWnd);

	SetWindowPos(hWnd,
		nullptr,
		pRect->left,
		pRect->top,
		pRect->right,
		pRect->bottom, 
		SWP_NOZORDER
	);

	m_pWorkArea->OnDPIChanged();

	return 0;
}

LRESULT AppWindow::OnCommand(HWND hWnd, WPARAM wParam)
{
	const WPARAM wIdentifier = LOWORD(wParam);

	if (wIdentifier >= ID_FILE_CLOSE && wIdentifier <= ID_FILE_RECENTFILES)
	{
		return HandleFileMenuCommands(hWnd, wIdentifier);
	}

	return 0;
}

LRESULT AppWindow::HandleFileMenuCommands(HWND hWnd, WPARAM wIdentifier)
{
	switch (wIdentifier)
	{
	case ID_FILE_OPEN_FOLDER:
		return OnOpenFolder(hWnd);

	case ID_FILE_OPEN_FILE:
		return OnOpenFile();

	case ID_FILE_CLOSE:
		return OnCloseProject();

	case ID_FILE_EXIT:
		PostQuitMessage(0);
		return 0;

	case ID_FILE_SAVEFILE:
		m_pExplorer->SaveCurrentFile(m_pWorkArea);
		return 0;

	case ID_FILE_SAVEALL:
		m_pExplorer->SaveAllFiles(m_pWorkArea);
		return 0;
	}

	return 0;
}

LRESULT AppWindow::OnOpenFile(void)
{
	wchar_t buffer[MAX_PATH] = { 0 };

	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = m_hWndSelf;
	ofn.lpstrFilter = L"Source Code Files (*.*)\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFile = buffer;
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_NONETWORKBUTTON | OFN_PATHMUSTEXIST;
	ofn.lpstrDefExt = L".txt";

	m_pStatusBar->SetText(L"Browsing files...", 0);

	if (GetOpenFileName(&ofn))
	{
		m_pWorkArea->CreateTab(ofn.lpstrFile);
		m_pStatusBar->SetText(L"File opened.", 0);
	}

	else 
	{
		m_pStatusBar->SetText(L"Browsing operation cancelled.", 0);
	}

	return 0;
}

static INT CALLBACK BrowseFolderCallback(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	if (uMsg == BFFM_INITIALIZED) 
	{
		::SendMessage(hwnd, BFFM_SETSELECTION, true, lpData);
	}

	return 0;
}

LRESULT AppWindow::OnOpenFolder(HWND hWnd)
{
	m_pStatusBar->SetText(L"Browsing Folders...", 0);

	BROWSEINFO bInfo;
	ZeroMemory(&bInfo, sizeof(bInfo));
	bInfo.lpszTitle = L"Select the project folder";
	bInfo.hwndOwner = hWnd;
	bInfo.lParam = reinterpret_cast<LPARAM>(L"C:\\");
	bInfo.lpfn = BrowseFolderCallback;
	bInfo.ulFlags = BIF_NEWDIALOGSTYLE;

	LPITEMIDLIST pidl = SHBrowseForFolder(&bInfo);

	if (pidl != NULL)
	{
		wchar_t path[MAX_PATH];
		SHGetPathFromIDList(pidl, path);

		IMalloc* pIMalloc = nullptr;

		if (SUCCEEDED(SHGetMalloc(&pIMalloc))) 
		{
			pIMalloc->Free(pidl);
			pIMalloc->Release();
		}
	
		m_pStatusBar->SetText(L"Closing Project...", 0);
		m_pWorkArea->CloseAllTabs();
		m_pStatusBar->SetText(L"Opening project folder...", 0);
		m_pExplorer->OpenProjectFolder(path);
		m_pStatusBar->SetText(L"Folder successfully opened.", 0);
	}

	else {
		m_pStatusBar->SetText(L"Browsing operation cancelled.", 0);
	}

	return 0;
}

LRESULT AppWindow::OnCloseProject(void)
{
	const wchar_t* lpszCloseMessage = L"Closing files...";
	const wchar_t* lpszSuccessMessage = L"All files closed.";

	if (TreeView_GetCount(m_pExplorer->GetTreeHandle()) > 0) {
		lpszCloseMessage = L"Closing folders and files...";
		lpszSuccessMessage = L"All folders and files closed.";
	}

	m_pStatusBar->SetText(lpszCloseMessage, 0);
	m_pExplorer->CloseProjectFolder();

	if (!m_pWorkArea->GetVisibleTabs().empty() || !m_pWorkArea->GetHiddenTabs().empty())
	{
		m_pWorkArea->CloseAllTabs();
	}

	m_pStatusBar->SetText(lpszSuccessMessage, 0);

	return 0;
}

static LRESULT OnCreate(HWND hWnd, LPARAM lParam)
{
	LPCREATESTRUCT lpParams = reinterpret_cast<LPCREATESTRUCT>(lParam);
	
	SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG>(lpParams->lpCreateParams));

	Utility::UpdateFont(hWnd);

	return 0;
}

static LRESULT CALLBACK AppWindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uMessage)
	{
	case WM_CREATE:
		return OnCreate(hWnd, lParam);

	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;

	case WM_QUIT:
		Logger::CloseOutputFile();
		return 0;

	default:
		AppWindow* pAppWindow = GetAssociatedObject<AppWindow>(hWnd);

		if (pAppWindow != nullptr)
		{
			return pAppWindow->WindowProcedure(hWnd, uMessage, wParam, lParam);
		}
	}

	return DefWindowProc(hWnd, uMessage, wParam, lParam);
}