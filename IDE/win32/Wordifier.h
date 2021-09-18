#pragma once

#include <vector>
#include <string>

typedef std::vector<std::wstring> word_list_t;

/* Reads a string and creates a vector consisting of its words */
class Wordifier
{
private:
	word_list_t m_Words;

public:
	explicit Wordifier(const std::wstring&);

	void Wordify(const std::wstring&);

	const word_list_t& GetWords(void) const;
};

