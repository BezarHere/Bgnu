#pragma once
#include "Range.hpp"
#include "base.hpp"

enum class CodeTokenType
{

};

struct CodeToken
{
	struct CodePosition
	{
		std::streampos line;
		std::streamoff column;
	};
	CodeTokenType type;


	CodePosition position;
};

struct CodeTokenizer
{
	static constexpr string_char escape = '\\';

	static size_t find_strings(const StrBlob &source, std::vector<IndexRange> &ranges);

	// next position of the character that matches predicate and not escaped
	// returns -1 if no character is found
	// checks the entire source, ignoring nulls
	template <typename Pred>
	inline static size_t match_unescaped(const StrBlob &source, Pred &&predicate);

};

template<typename Pred>
inline size_t CodeTokenizer::match_unescaped(const StrBlob &source, Pred &&predicate) {
	for (size_t i = 0; i < source.size; i++)
	{
		// skip escaped
		if (source[i] == escape)
		{
			i++;
			continue;
		}

		if (predicate(source[i]))
		{
			return i;
		}
	}

	return npos;
}
