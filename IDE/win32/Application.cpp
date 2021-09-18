#include "Application.h"
#include "Logger.h"
#include "Explorer.h"

Application::Application(HINSTANCE hInstance)
{

}

HRESULT Application::Initialize(void)
{
	HMODULE hModule = LoadLibrary(L"Msftedit.dll");

	if (hModule == nullptr)
	{
		Logger::Write(L"Failed to load Msftedit.dll!");
		return E_FAIL;
	}

	m_AppWindow.Initialize(GetModuleHandle(NULL));

	return S_OK;
}

void Application::Run(void)
{
	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}