#pragma once
#include "base.hpp"


struct StringUtils
{
	using string_type = std::string;

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

	static inline string_type to_lower(const string_type &str) {
		return _modify(str, [](char c) { return _tolower(c); });
	}

	static inline string_type to_upper(const string_type &str) {
		return _modify(str, [](char c) { return _toupper(c); });
	}

};
