#include "CodeTokenizer.hpp"



size_t CodeTokenizer::find_strings(const StrBlob &source, std::vector<IndexRange> &ranges) {
	size_t last = 0;
	while (last < source.size && last != npos)
	{
		last = match_unescaped(
			source.slice(last),
			[](string_char value) { return value == '"'; }
		);
	}

	return 0;
}