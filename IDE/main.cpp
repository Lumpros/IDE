#include "Application.h"
#include "Logger.h"
#include "Utility.h"

#include <CommCtrl.h>

#pragma comment(lib, "ComCtl32.lib")

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' " \
	"version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

INT APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	              _In_opt_ HINSTANCE hPrevInstance,
	                  _In_ LPWSTR    lpCmdLine,
	                  _In_ int       nCmdShow
)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	InitCommonControls();

	Logger::SetOutputPath(L"logs.txt");

	Application m_App(hInstance);

	if (FAILED(m_App.Initialize()))
	{
		Logger::Write(L"App initialization failed!");
		Logger::CloseOutputFile();
		return -1;
	}

	m_App.Run();

	return 0;
}