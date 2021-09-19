#include "Application.h"
#include "Logger.h"
#include "Explorer.h"
#include "resource.h"

Application::Application(HINSTANCE hInstance)
{
	m_hAccelerator = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));
}

HRESULT Application::Initialize(LPWSTR lpCmdLine)
{
	HMODULE hModule = LoadLibrary(L"Msftedit.dll");

	if (hModule == nullptr)
	{
		Logger::Write(L"Failed to load Msftedit.dll!");
		return E_FAIL;
	}

	m_AppWindow.Initialize(GetModuleHandle(NULL), lpCmdLine);

	return S_OK;
}

void Application::Run(void)
{
	MSG msg;

	HWND hWnd = m_AppWindow.GetHandle();

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(hWnd, m_hAccelerator, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}