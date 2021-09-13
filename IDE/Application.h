#pragma once

#include "AppWindow.h"

class Application
{
private:
	AppWindow m_AppWindow;

public:
	explicit Application(HINSTANCE hInstance);

	HRESULT Initialize(void);

	void Run(void);
};

