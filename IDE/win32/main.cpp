#include "Application.h"
#include "Logger.h"
#include "Utility.h"

#include <CommCtrl.h>
#include <Uxtheme.h>

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
	UNREFERENCED_PARAMETER(nCmdShow);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	Logger::SetOutputPath(L"logs.txt");

	InitCommonControls();

	if (SUCCEEDED(BufferedPaintInit()))
	{
		Application m_App(hInstance);

		if (FAILED(m_App.Initialize(lpCmdLine)))
		{
			Logger::Write(L"App initialization failed!");
			Logger::CloseOutputFile();
			return -1;
		}

		m_App.Run();
		BufferedPaintUnInit();
	}

	else
	{
		Logger::Write(L"Failed to initialize buffered paint!");
	}

	return 0;
}