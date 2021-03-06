#pragma once

#include "Window.h"
#include "Explorer.h"
#include "WorkArea.h"
#include "StatusBar.h"
#include "OutputContainer.h"

#define FIND_BUFFER_SIZE 128

class AppWindow : public Window
{
private:
	HRESULT InitializeWindow(HINSTANCE hInstance);
	Explorer* m_pExplorer = nullptr;
	WorkArea* m_pWorkArea = nullptr;
	OutputContainer* m_pOutputContainer = nullptr;
	StatusBar* m_pStatusBar = nullptr;
	
	wchar_t replace_buffer[FIND_BUFFER_SIZE];
	wchar_t find_buffer[FIND_BUFFER_SIZE];

	LRESULT OnSize(HWND hWnd, LPARAM lParam);
	LRESULT OnGetMinMax(HWND hWnd, LPARAM lParam);
	LRESULT OnDPIChanged(HWND hWnd, LPARAM lParam);
	LRESULT OnCommand(HWND hWNd, WPARAM wParam);
	LRESULT OnOpenFolder(HWND hWnd);
	LRESULT OnCloseProject(void);
	LRESULT OnOpenFile(void);
	LRESULT OnViewStatusBar(void);
	void OnSelectAll(HWND hEditWnd);
	void OnFind(void);
	void OnReplace(void);
	void HandleFRMessage(HWND hWnd, WPARAM wParam, LPARAM lParam);

	HRESULT InitializeComponents(void);

	LRESULT HandleFileMenuCommands(HWND hWnd, WPARAM wIdentifier);
	LRESULT HandleViewMenuCommands(HWND hWnd, WPARAM wIdentifier);
	LRESULT HandleEditMenuCommands(HWND hWnd, WPARAM wIdentifier);

	void OpenFolderFromCommandLine(LPWSTR lpCmdLine);
	void RefreshChildPositions(void);

public:
	~AppWindow(void);

	void Initialize(HINSTANCE hInstance, LPWSTR lpCmdLine);
	HWND* m_pFindDialog = nullptr;

	LRESULT WindowProcedure(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam);

	inline WorkArea* GetWorkArea(void) const {
		return m_pWorkArea;
	}

	inline Explorer* GetExplorer(void) const {
		return m_pExplorer;
	}

	inline OutputContainer* GetOutputWindow(void) const {
		return m_pOutputContainer;
	}

	inline StatusBar* GetStatusBar(void) const {
		return m_pStatusBar;
	}
};

