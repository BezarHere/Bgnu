#pragma once
#include "base.hpp"
#include <string.h>


struct StringUtils
{
	using string_type = std::string;
	using char_type = string_type::value_type;
	using traits_type = string_type::traits_type;

	template <typename StrProcessor>
	static ALWAYS_INLINE string_type _modify(const string_type &str, const StrProcessor &processor = {}) {
		if (str.empty())
		{
			return "";
		}

		string_type result{};
		result.resize(str.size());

		for (size_t i = 0; i < str.length(); ++i)
		{
			result[i] = processor(str[i]);
		}

		return result;
	}

	template <typename Comparer = std::equal_to<char_type>>
	static ALWAYS_INLINE bool _equal(const char_type *left, const char_type *right,
																	 const size_t max_count, const Comparer &comparer = {}) {
		if (max_count == 0)
		{
			return true;
		}

		for (size_t i = 0; i < max_count; ++i)
		{
			// comparer should return true if the strings match at 'i'
			// if the string don't match, then they are not equal
			if (!comparer(left[i], right[i]))
			{
				return false;
			}

			// if one string ended before the other
			// then 'comparer' should have returned false (left[i] > 0, right[i] == 0)
			// if 'comparer' is fine with the two strings having different length, then be it!
			if (left[i] == 0 || right[i] == 0)
			{
				break;
			}
		}

		return true;
	}

	static inline string_type to_lower(const string_type &str) {
		return _modify(str, [](char_type c) { return (char_type)tolower(c); });
	}

	static inline string_type to_upper(const string_type &str) {
		return _modify(str, [](char_type c) { return (char_type)toupper(c); });
	}

	static inline bool equal(const char_type *left, const char_type *right, const size_t max_count = npos) {
		return _equal(left, right, max_count, std::equal_to{});
	}

	static inline bool equal_insensitive(const char_type *left, const char_type *right, const size_t max_count = npos) {
		return _equal(
			left, right,
			max_count,
			[](const char_type left_c, const char_type right_c) { return tolower(left_c) == tolower(right_c); }
		);
	}

};
