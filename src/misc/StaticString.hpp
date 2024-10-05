#pragma once
#include <string>
#include <stdexcept>

// Pneumonoultramicroscopicsilicovolcanoconiosis
constexpr size_t StaticStr_LongestWordLength = 45;

template <size_t _MaxLen, typename _T>
class BasicStaticString
{
public:
	static constexpr size_t max_str_length = _MaxLen;

	typedef _T value_type;
	typedef _T char_type;
	typedef std::basic_string<value_type> string_type;
	typedef typename string_type::traits_type traits_type;
	typedef BasicStaticString<_MaxLen, _T> this_type;

	typedef char_type container_type[max_str_length + 1];

	constexpr void _copy(char_type *dst, const char_type *src, size_t length = max_str_length);
	constexpr size_t _length(const char_type *src, size_t max_length = max_str_length);

	static constexpr bool _equality(const container_type &left, const container_type &right);
	static constexpr bool _equality(const char_type *left,
																	const char_type *right,
																	size_t max_length = max_str_length);

	constexpr BasicStaticString() = default;

	constexpr BasicStaticString(std::nullptr_t)
		: BasicStaticString() {
	}

	constexpr BasicStaticString(const char_type *cstr, const size_t length)
		: m_length{std::min(max_str_length, length)} {
		(void)_copy(m_data, cstr, m_length);
		m_data[m_length] = char_type();
	}

	constexpr BasicStaticString(const char_type *cstr)
		: BasicStaticString(cstr, _length(cstr)) {
	}

	constexpr explicit BasicStaticString(const char_type _char)
		: m_length{_char != char_type()} {
		// filling the string with null chars
		if (m_length)
		{
			m_data[m_length - 1] = _char;
		}

		m_data[m_length] = char_type();
	}

	constexpr explicit BasicStaticString(const size_t count, const char_type _char)
		: m_length{std::min(max_str_length, count)} {
		// filling the string with null chars
		if (_char == char_type())
		{
			m_length = 0;
		}
		else
		{
			for (size_t i = 0; i < m_length; i++)
			{
				m_data[i] = _char;
			}
		}

		m_data[m_length] = char_type();
	}

	template <size_t N>
	constexpr BasicStaticString(const char_type(&_arr)[N])
		: m_length{std::min(max_str_length, N)} {
		// filling the string with null chars
		for (size_t i = 0; i < std::min(max_str_length, N); i++)
		{
			if (_arr[i] == char_type())
			{
				m_length = i;
				break;
			}

			m_data[i] = _arr[i];
		}

		m_data[m_length] = char_type();
	}

	constexpr BasicStaticString(const string_type &str)
		: BasicStaticString(str.c_str(), str.length()) {
	}

	template <size_t SN>
	inline bool operator==(const BasicStaticString<SN, _T> &other) const {
		if (size() != other.size())
		{
			return false;
		}

		return _equality(this->m_data, other.m_data, std::min(size(), other.size()));
	}

	inline bool operator==(const std::basic_string<char_type> &other) const {
		if (size() != other.size())
		{
			return false;
		}

		return _equality(this->m_data, other.m_data, std::min(size(), other.size()));
	}

	inline bool operator==(const char_type *other) const {
		return _equality(this->m_data, other, size());
	}

	template <size_t SN>
	inline bool operator!=(const BasicStaticString<SN, _T> &other) const {
		return !(*this == other);
	}

	template <size_t SN>
	inline bool operator!=(const std::basic_string<char_type> &other) const {
		return !(*this == other);
	}

	inline bool operator!=(const char_type *other) const {
		return !(*this == other);
	}

	inline operator string_type() const {
		return string_type(m_data, m_length);
	}

	inline auto operator+(const string_type &other) const -> decltype(string_type() + other) {
		return string_type(m_data, m_length) + other;
	}

	inline auto operator+(const string_char *other) const -> decltype(string_type() + other) {
		return string_type(m_data, m_length) + other;
	}

	// static inline auto operator+(const string_type &left, const this_type &right)
	// 	-> decltype(string_type() + string_type()) {
	// 	return left + string_type(right);
	// }

	// static inline auto operator+(const string_char *left, const this_type &right)
	// 	-> decltype(string_type() + string_type()) {
	// 	return left + string_type(right);
	// }

	inline constexpr char_type &operator[](size_t pos) {
		if (pos > m_length)
		{
			throw std::out_of_range("pos");
		}

		return m_data[pos];
	}

	inline constexpr char_type operator[](size_t pos) const {
		if (pos > m_length)
		{
			throw std::out_of_range("pos");
		}

		return m_data[pos];
	}

	inline constexpr size_t length() const {
		return m_length;
	}
	inline constexpr size_t size() const {
		return m_length;
	}

	inline constexpr bool empty() const {
		return m_length == 0;
	}

	inline constexpr value_type *data() {
		return m_data;
	}
	inline constexpr const value_type *data() const {
		return m_data;
	}
	inline constexpr const value_type *c_str() const {
		return m_data;
	}

	inline constexpr value_type *begin() {
		return m_data;
	}
	inline constexpr value_type *end() {
		return m_data + m_length;
	}

	inline constexpr const value_type *begin() const {
		return m_data;
	}
	inline constexpr const value_type *end() const {
		return m_data + m_length;
	}

private:
	size_t m_length = 0;
	container_type m_data = {0};
};

template <size_t MaxLen>
using StaticString = BasicStaticString<MaxLen, char>;
template <size_t MaxLen>
using WStaticString = BasicStaticString<MaxLen, wchar_t>;



template<size_t _MaxLen, typename _T>
inline constexpr void BasicStaticString<_MaxLen, _T>::_copy(char_type *dst, const char_type *src, size_t length) {
	for (size_t i = 0; i < length && src[i]; i++)
	{
		dst[i] = src[i];
	}
}

template<size_t _MaxLen, typename _T>
inline constexpr size_t BasicStaticString<_MaxLen, _T>::_length(const char_type *src, size_t max_length) {
	for (size_t i = 0; i < max_length; i++)
	{
		if (src[i] == char_type())
		{
			return i;
		}
	}

	return max_length;
}

template<size_t _MaxLen, typename _T>
inline constexpr bool BasicStaticString<_MaxLen, _T>::_equality(const container_type &left, const container_type &right) {
	constexpr size_t min_size = std::min(_MaxLen, _MaxLen);

	for (size_t i = 0; i < min_size; i++)
	{
		if (left[i] != right[i])
		{
			return false;
		}

		// right[i] will also equal 0, given above passes
		if (left[i] == 0)
		{
			break;
		}
	}

	return true;
}

template<size_t _MaxLen, typename _T>
inline constexpr bool BasicStaticString<_MaxLen, _T>::_equality(const char_type *left, const char_type *right, size_t max_length) {

	for (size_t i = 0; i < max_length; i++)
	{
		if (left[i] != right[i])
		{
			return false;
		}

		// right[i] will also equal 0, given above passes
		if (left[i] == 0)
		{
			break;
		}
	}

	return true;
}
