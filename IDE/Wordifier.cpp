#include "Wordifier.h"

Wordifier::Wordifier(const std::wstring& wstr)
{
	Wordify(wstr);
}

void Wordifier:: Wordify(const std::wstring& wstr)
{
	m_Words.clear();

	const size_t length = wstr.length();
	std::wstring next_word;

	for (size_t i = 0; i < length; ++i)
	{
		/* Skip spaces until we find a letter */
		while (i < length && iswspace(wstr[i]))
		{
			++i;
		}
		
		/* Add letters to string until we encounter a whitespace */
		while (i < length && !iswspace(wstr[i]))
		{
			next_word.push_back(wstr[i++]);
		}

		m_Words.push_back(next_word);
		next_word.clear();
	}
}

const word_list_t& Wordifier::GetWords(void) const
{
	return m_Words;
}