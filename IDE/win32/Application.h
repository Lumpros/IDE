#pragma once

#include "AppWindow.h"

class Application
{
private:
	AppWindow m_AppWindow;
	HACCEL m_hAccelerator = nullptr;

public:
	Application(HINSTANCE hInstance);

	HRESULT Initialize(LPWSTR lpCmdLine);

	void Run(void);
};

