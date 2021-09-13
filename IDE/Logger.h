#pragma once

#define LOGGER_ACTIVE

namespace Logger
{
	void SetOutputPath(const wchar_t* lpszFilePath);

	/* Enters a new line character after the text*/
	void Write(const wchar_t* lpszFormat, ...);

	void CloseOutputFile(void);
}