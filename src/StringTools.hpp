#pragma once
#include <string.h>

#include <concepts>

#include "Logger.hpp"
#include "Range.hpp"
#include "base.hpp"
#include "misc/SpellChecker.hpp"

namespace string_tools
{
  constexpr size_t npos = std::string::npos;

  using string_type = string;
  using char_type = string_char;
  using traits_type = string_type::traits_type;

  using wide_string_type = std::wstring;
  using wide_char_type = wide_string_type::value_type;
  using wide_traits_type = wide_string_type::traits_type;

  // multi-byte character length (utf-8) (loose)
  extern uint8_t MBCLength(const string_char character);
  // multi-byte string length (utf-8) (loose)
  extern size_t MBStrLength(const string_char *str, const size_t max_length = npos);

  template <typename StrProcessor>
  static ALWAYS_INLINE string_type _modify(const string_type &str,
                                           const StrProcessor &processor = {}) {
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
  static ALWAYS_INLINE constexpr bool _equal(const char_type *left, const char_type *right,
                                             const size_t max_count,
                                             const Comparer &comparer = {}) {
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

  // copy the chars from `src` to `dst` until a null is hit or max_count is reached
  // will NOT put any null char in `dst`
  static inline constexpr size_t copy(char_type *dst, const char_type *src, size_t max_count) {
    size_t index = 0;
    for (; index < max_count && src[index] != 0; index++)
    {
      dst[index] = src[index];
    }

    return index;
  }

  static inline string_type to_lower(const string_type &str) {
    return _modify(str, [](char_type c) { return (char_type)tolower(c); });
  }

  static inline string_type to_upper(const string_type &str) {
    return _modify(str, [](char_type c) { return (char_type)toupper(c); });
  }

  static constexpr bool equal(const char_type *left, const char_type *right,
                              const size_t max_count = npos) {
    return _equal(left, right, max_count, std::equal_to{});
  }

  static constexpr bool equal_insensitive(const char_type *left, const char_type *right,
                                          const size_t max_count = npos) {
    return _equal(left, right, max_count, [](const char_type left_c, const char_type right_c) {
      return tolower(left_c) == tolower(right_c);
    });
  }

  template <typename CountPred>
  static inline size_t count(const char_type *start, const size_t max_count, CountPred &&pred) {
    for (size_t i = 0; i < max_count && start[i]; i++)
    {
      if (!pred(start[i]))
      {
        return i;
      }
    }
    return max_count;
  }

  // `count_` needs to be the length of the string, unlike count's `max_count` argument
  template <typename CountPred>
  static inline size_t rcount(const char_type *start, const size_t count_, CountPred &&pred) {
    for (size_t i = 1; i <= count_; i++)
    {
      if (!pred(start[count_ - i]))
      {
        return i - 1;
      }
    }
    return count_;
  }

  static constexpr inline size_t length(const char_type *start, const size_t max_count = npos) {
    for (size_t i = 0; i < max_count; i++)
    {
      if (start[i] == 0)
      {
        return i;
      }
    }

    return max_count;
  }

  static inline string_type narrow(const wide_char_type *src_str, const size_t max_count) {
    const size_t dst_sz = wcsnlen_s(src_str, max_count) * sizeof(*src_str) + 1;
    string_type dst_str{};
    dst_str.resize(dst_sz);

    mbstate_t mb_state = { 0 };
    size_t chars_converted = wcsnrtombs(dst_str.data(), &src_str, max_count, dst_sz, &mb_state);
    errno_t error = errno;

    if (error != 0)
    {
      Logger::error("Error while narrowing string \"%S\" max count=%llu, errno=%d", src_str,
                    max_count, error);
    }

    dst_str.resize(strnlen(dst_str.c_str(), dst_str.size()));

    return dst_str;
  }

  static inline wide_string_type widen(const char_type *src_str, const size_t max_count) {
    const size_t dst_sz = MBStrLength(src_str, max_count) * sizeof(*src_str) + 1;
    wide_string_type dst_str{};
    dst_str.resize(dst_sz);

    mbstate_t mb_state = { 0 };
    size_t chars_converted = mbsnrtowcs(dst_str.data(), &src_str, max_count, dst_sz, &mb_state);
    errno_t error = errno;

    if (error != 0)
    {
      Logger::error("Error while widening string \"%S\" max count=%llu, error=%s", dst_str.c_str(),
                    max_count, GetErrorName(error));
    }

    dst_str.resize(wcsnlen(dst_str.c_str(), dst_str.size()));

    return dst_str;
  }

  template <typename CHR>
  static inline std::basic_string<CHR> convert(const char_type *src_str, const size_t max_count);

  template <>
  inline string_type convert(const char_type *src_str, const size_t max_count) {
    return string_type(src_str, strnlen(src_str, max_count));
  }

  template <>
  inline wide_string_type convert(const char_type *src_str, const size_t max_count) {
    return widen(src_str, max_count);
  }

  template <typename CHR>
  static inline std::basic_string<CHR> convert(const wide_char_type *src_str,
                                               const size_t max_count);

  template <>
  inline string_type convert(const wide_char_type *src_str, const size_t max_count) {
    return narrow(src_str, max_count);
  }

  template <>
  inline wide_string_type convert(const wide_char_type *src_str, const size_t max_count) {
    return wide_string_type(src_str, wcsnlen(src_str, max_count));
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
      return { source.size, source.size };
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

    return { leading, source.size - trailing };
  }

  static ALWAYS_INLINE IndexRange printable_range(const StrBlob &source) {
    return range(source, &isprint);
  }

  static ALWAYS_INLINE IndexRange whitespace_range(const StrBlob &source) {
    return range(source, &iswspace);
  }

  static constexpr ALWAYS_INLINE bool is_directory_separator(const char_type character) {
    return character == '\\' || character == '/';
  }

  static constexpr ALWAYS_INLINE bool is_whitespace(const char_type character) {
    return iswspace(character);
  }

  static constexpr ALWAYS_INLINE bool is_printable(const char_type character) {
    return isprint(character);
  }

  static constexpr ALWAYS_INLINE bool is_newline(const char_type character) {
    return character == '\n' || character == '\r';
  }

  static constexpr ALWAYS_INLINE bool is_eof(const char_type character) { return character == 0; }

  static constexpr ALWAYS_INLINE bool is_ascii(const char_type character) {
    return character < 0x7f;
  }

  static constexpr ALWAYS_INLINE bool is_lowercase(const char_type character) {
    return character >= 'a' && character <= 'z';
  }

  static constexpr ALWAYS_INLINE bool is_uppercase(const char_type character) {
    return character >= 'A' && character <= 'Z';
  }

  template <typename _Pred>
  static inline StrBlob trim(const StrBlob &source, _Pred &&trim_pred) {
    size_t leading_c = count(source.begin(), source.length(), trim_pred);

    if (leading_c >= source.size)
    {
      return StrBlob(source.end(), (size_t)0);
    }

    size_t trailing_c = rcount(source.begin() + leading_c, source.length() - leading_c, trim_pred);

    if (trailing_c + leading_c >= source.size)
    {
      return StrBlob(source.begin() + leading_c, (size_t)0);
    }

    return { source.begin() + leading_c, source.end() - trailing_c };
  }

  static inline StrBlob trim(const StrBlob &source) { return trim(source, &is_whitespace); }

  template <typename _Pred>
  static constexpr bool all_of(const char_type *cstr, const size_t max_count, _Pred &&pred) {
    for (size_t i = 0; i < max_count && cstr[i]; i++)
    {
      if (!pred(cstr[i]))
      {
        return false;
      }
    }
    return true;
  }

  template <typename _Pred>
  static inline constexpr bool all_of(const char_type *cstr, _Pred &&pred) {
    return all_of(cstr, npos, std::forward<_Pred>(pred));
  }

  template <typename _Pred>
  static constexpr bool any_of(const char_type *cstr, const size_t max_count, _Pred &&pred) {
    for (size_t i = 0; i < max_count && cstr[i]; i++)
    {
      if (pred(cstr[i]))
      {
        return true;
      }
    }
    return false;
  }

  template <typename _Pred>
  static inline constexpr bool any_of(const char_type *cstr, _Pred &&pred) {
    return any_of(cstr, npos, std::forward<_Pred>(pred));
  }

  template <typename Predicate>
  static inline constexpr size_t find(const char_type *str, const size_t max_length,
                                      Predicate &&pred) {
    for (size_t i = 0; i < max_length; i++)
    {
      if (pred(str[i]))
      {
        return i;
      }

      if (str[i] == 0)
      {
        return npos;
      }
    }
    return npos;
  }

  template <typename Predicate>
  static inline constexpr size_t find_last(const char_type *str, const size_t max_length,
                                           Predicate &&pred) {
    size_t last_find = npos;
    for (size_t i = 0; i < max_length; i++)
    {
      if (pred(str[i]))
      {
        last_find = i;
      }

      if (str[i] == 0)
      {
        break;
      }
    }

    return last_find;
  }

  static inline constexpr size_t find(const char_type *str, char_type chr,
                                      const size_t max_length) {
    return find(str, max_length, [chr](char_type chr2) { return chr == chr2; });
  }

  static inline constexpr size_t find_last(const char_type *str, char_type chr,
                                           const size_t max_length) {
    return find_last(str, max_length, [chr](char_type chr2) { return chr == chr2; });
  }

  template <typename CHR>
  static inline std::basic_string<CHR> replace(const std::basic_string<CHR> &source,
                                               const std::basic_string<CHR> &target,
                                               const std::basic_string<CHR> &replacement) {
    if (target.empty() || source.empty())
    {
      return source;
    }

    std::basic_string<CHR> result{};

    for (size_t i = 0; i < source.size(); i++)
    {
      const auto target_match_len = std::min(source.size() - i, target.size());
      if (equal(source.data() + i, target.c_str(), target_match_len))
      {
        if (!replacement.empty())
        {
          result.append(replacement);
        }

        i += target.size() - 1;
        continue;
      }

      result.append(1, source[i]);
    }

    return result;
  }
};
