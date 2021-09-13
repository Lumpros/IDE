#pragma once

#define WIN32_LEAN_AND_MEAN

#include <unordered_map>
#include <Windows.h>

struct CRSTATUS {
	COLORREF cr = 0;
	bool wasFound = false;
};

class ColorFormatParser
{
private:
	std::unordered_map<std::wstring, COLORREF> m_ColorMap;

public:
	void ParseFile(const wchar_t* lpszFileName);

	/* If the keyword exists, then the color is returned */
	/* Otherwise the wasFound variable is set to false and cr is equal to 0*/
	/* This is done so the program will try to identify the word as something else */
	/* e.g. a variable to use a different color */
	CRSTATUS GetKeywordColor(const wchar_t* lpszKeyword);
};

