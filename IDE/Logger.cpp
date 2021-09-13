#define _CRT_SECURE_NO_WARNINGS

#include "Logger.h"

#include <stdio.h>
#include <stdarg.h>
#include <chrono>
#include <ctime>

#define WIN32_LEAN_AND_MEAN

#include <Windows.h>

static FILE* pOutputFile = nullptr;
static const wchar_t* g_lpszFilePath = nullptr;

static void OpenOutputFile(void)
{
	_wfopen_s(&pOutputFile, g_lpszFilePath, L"a");

	if (!pOutputFile)
	{
		MessageBox(nullptr, L"Unable to create/open logger file!", L"Error", MB_OK | MB_ICONERROR);
	}
}

static void PrintSystemTime(void)
{
	const std::time_t now = std::time(nullptr);
	char* time_string = std::ctime(&now);

	time_string[strlen(time_string) - 1] = '\0'; /* Remove the newline character */

	fprintf(pOutputFile, "[%s] ", time_string);
}

void Logger::SetOutputPath(const wchar_t* lpszFilePath)
{
#ifdef LOGGER_ACTIVE

	g_lpszFilePath = lpszFilePath;

#endif
}

void Logger::Write(const wchar_t* lpszFormat, ...)
{
#ifdef LOGGER_ACTIVE

	OpenOutputFile();

	if (pOutputFile)
	{
		va_list args;
		va_start(args, lpszFormat);

		PrintSystemTime();
		vfwprintf_s(pOutputFile, lpszFormat, args);

		va_end(args);

		fputc('\n', pOutputFile);

		/* Close it after we're done so the user can read the logs while the program is running */
		Logger::CloseOutputFile();
	}

#endif
}

void Logger::CloseOutputFile(void)
{
#ifdef LOGGER_ACTIVE

	if (pOutputFile)
	{
		fclose(pOutputFile);
		pOutputFile = nullptr;
	}

#endif
}