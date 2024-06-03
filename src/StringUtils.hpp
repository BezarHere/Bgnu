#pragma once
#include "base.hpp"
#include "Range.hpp"
#include <string.h>
#include "misc/SpellChecker.hpp"
#include "Logger.hpp"


struct StringUtils
{
	using string_type = string;
	using char_type = string_char;
	using traits_type = string_type::traits_type;

	using wide_string_type = std::wstring;
	using wide_char_type = wide_string_type::value_type;
	using wide_traits_type = wide_string_type::traits_type;

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

	template <typename CountPred>
	static inline size_t count(const char_type *start, const size_t max_count, CountPred &&pred) {
		for (size_t i = 0; i < max_count; i++)
		{
			if (!pred(start[i]))
			{
				return i;
			}
		}
		return max_count;
	}

	static inline string_type narrow(const wide_char_type *src_str, const size_t max_count) {
		const size_t dst_sz = wcsnlen_s(src_str, max_count) * 2 + 1;
		string_type dst_str{};
		dst_str.resize(dst_sz);

		size_t chars_converted = 0;
		errno_t error =
			wcstombs_s(&chars_converted, dst_str.data(), dst_sz * sizeof(char_type), src_str, max_count);

		if (error != 0)
		{
			Logger::error(
				"Error while narrowing string \"%S\" max count=%llu, errno=%d",
				src_str,
				max_count,
				error
			);
		}

		return dst_str;
	}

	static inline wide_string_type widen(const char_type *src_str, const size_t max_count) {
		const size_t dst_sz = mblen(src_str, max_count) + 1;
		wide_string_type dst_str{};
		dst_str.resize(dst_sz);

		size_t chars_converted = 0;
		errno_t error =
			mbstowcs_s(&chars_converted, dst_str.data(), dst_sz * sizeof(wide_char_type), src_str, max_count);

		if (error != 0)
		{
			Logger::error(
				"Error while widening string \"%S\" max count=%llu, errno=%d",
				dst_str,
				max_count,
				error
			);
		}

		return dst_str;
	}

	template <class _Str>
	static inline _Str convert(const _Str::value_type *src_str, const size_t max_count) {
		(void)max_count;
		return _Str(src_str);
	}

	static inline wide_string_type convert(const char_type *src_str, const size_t max_count) {
		return widen(src_str, max_count);
	}

	static inline string_type convert(const wide_char_type *src_str, const size_t max_count) {
		return narrow(src_str, max_count);
	}

	/// @brief 
	/// @param first 
	/// @param second 
	/// @returns a value ranging from 0.0F to 1.0F representing the string's similarity 
	static inline float similarity(const string_type &first, const string_type &second) {
		int32_t distance = (int32_t)spell::wagner_fischer_distance(first, second);

		const auto max_distance = std::max(first.size(), second.size());

		return 1.0F - (float(distance) / max_distance);
	}

	/// @brief returns an inner range for the source,
	/// @brief skipping leading and trailing chars matching `pred`
	/// @tparam _Pred predicate type
	/// @param source the source string
	/// @param pred predicate to match the range
	/// @returns a range skipping any leading/trailing chars matching pred 
	template <typename _Pred>
	static inline IndexRange range(const StrBlob &source, _Pred &&pred) {

		// count of leading chars matching `pred`
		size_t leading = source.size;

		for (size_t i = 0; i < source.size; i++)
		{
			if (pred(source[i]))
			{
				leading++;
			}

			break;
		}

		if (leading == source.size)
		{
			return {source.size, source.size};
		}

		// count of trailing chars matching `pred`
		size_t trailing = 0;

		for (size_t i = source.size; i > leading; i--)
		{
			const size_t rindex = (i - 1);

			if (pred(source[rindex]))
			{
				trailing++;
			}

			break;
		}

		return {leading, source.size - trailing};
	}

	static ALWAYS_INLINE IndexRange printable_range(const StrBlob &source) {
		return range(source, &isprint);
	}

	static ALWAYS_INLINE IndexRange whitespace_range(const StrBlob &source) {
		return range(source, &iswspace);
	}

};
