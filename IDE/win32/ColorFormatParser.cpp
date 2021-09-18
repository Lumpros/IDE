#include "ColorFormatParser.h"
#include "Wordifier.h"
#include "Logger.h"
#include "Utility.h"

#include <fstream>
#include <string>
#include <sstream>

/* Words are expected to be encountered in the following order: */
/* keyword number (r) number (g) number (b) */
/* e.g. int 0 255 0 */
void ColorFormatParser::ParseFile(const wchar_t* lpszFileName)
{
	std::wifstream t(lpszFileName);
	
	if (t.fail())
	{
		Logger::Write(L"Failed to open %ls! Please make sure the file exists.", lpszFileName);
		return;
	}

	std::wstringstream buffer;
	buffer << t.rdbuf();
	
	Wordifier wordifier(buffer.str());
	const word_list_t& list = wordifier.GetWords();

	/* if the number count is not a multiple of 4 then there is an error */
	if (list.size() % 4 != 0)
	{
		Logger::Write(L"Error while parsing %ls: One or more values are missing.", lpszFileName);
		return;
	}

	bool success = true;

	for (size_t i = 0; i < list.size(); i += 4)
	{
		/* If one of the values following the keyword aren't a valid integer */
		/* Don't enter the value into the list */
		if (!Utility::IsNumber(list[i + 1].c_str()) ||
			!Utility::IsNumber(list[i + 2].c_str()) ||
			!Utility::IsNumber(list[i + 3].c_str()))
		{
			Logger::Write(
				L"Error while parsing %ls: Was expecting 3 integers after keyword \'%ls\' but an invalid value was read instead",
				lpszFileName, list[i].c_str()
			);

			success = false;
		}

		else
		{
			m_ColorMap[list[i]] = RGB(
				_wtoi(list[i + 1].c_str()),
				_wtoi(list[i + 2].c_str()),
				_wtoi(list[i + 3].c_str())
			);
		}
	}
}

CRSTATUS ColorFormatParser::GetKeywordColor(const wchar_t* lpszKeyword)
{
	CRSTATUS status;

	if (m_ColorMap.find(lpszKeyword) != m_ColorMap.end()) // exists
	{
		status.cr = m_ColorMap[lpszKeyword];
		status.wasFound = true;
	}

	return status;
}