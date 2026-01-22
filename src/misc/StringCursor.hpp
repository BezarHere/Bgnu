#pragma once
#include <string>

template <typename _T>
class basic_string_cursor
{
public:
	typedef _T value_type;
	typedef _T char_type;
	typedef std::basic_string<char_type> string_type;
	typedef std::streamoff offset_type;

	typedef string_type::traits_type traits_type;

	basic_string_cursor() = default;
	basic_string_cursor(size_t size);
	
	basic_string_cursor(const char_type *str, size_t index);

	inline basic_string_cursor(const char_type *str)
		: basic_string_cursor(str, 0) {
	}

	template <size_t N>
	inline basic_string_cursor(const char_type(&str)[N]) : basic_string_cursor(str, N) {}
	inline basic_string_cursor(const string_type &str) : basic_string_cursor(str.c_str(), str.length()) {}
	~basic_string_cursor();

	basic_string_cursor(const basic_string_cursor &copy);
	basic_string_cursor(basic_string_cursor &&move) noexcept;

	basic_string_cursor &operator=(const basic_string_cursor &copy);
	basic_string_cursor &operator=(basic_string_cursor &&move) noexcept;

	// the string length after the cursor
	inline size_t size() const {
		return m_ref->len - m_index;
	}

	// is there a valid string after the cursor
	inline bool empty() const {
		return m_ref == nullptr || m_ref->len == 0 || m_index >= m_ref->len;
	}

	// the total string held (even behind the cursor's position)
	inline size_t total_size() const {
		return m_ref->len;
	}

	// same as position()
	inline size_t index() const noexcept {
		return m_index;
	}

	// same as index()
	inline size_t position() const noexcept {
		return m_index;
	}

	inline char_type *begin() { return m_ref->get() + m_index; }
	inline char_type *end() { return m_ref->get() + m_ref->len; }

	inline const char_type *begin() const { return m_ref->get() + m_index; }
	inline const char_type *end() const { return m_ref->get() + m_ref->len; }

	inline char_type &operator[](size_t pos) { return begin()[pos]; }
	inline char_type operator[](size_t pos) const { return begin()[pos]; }

	void move(offset_type offset);
	basic_string_cursor slice(size_t start) const;

	inline explicit operator string_type() const {
		if (empty()) return string_type();
		return string_type(begin(), size());
	}

	inline const char_type *c_str() const { return m_ref->get() + m_index; }
	inline char_type *data() { return m_ref->get() + m_index; }
	inline const char_type *data() const { return m_ref->get() + m_index; }

	basic_string_cursor operator+(const offset_type offset);
	inline basic_string_cursor operator-(const offset_type offset) { return *this + (-offset); }

	inline char_type *origin() { return m_ref->get(); }
	inline const char_type *origin() const { return m_ref->get(); }

private:
	struct string_ref
	{
		inline char_type *get() { return reinterpret_cast<char_type *>(this + 1); }
		inline const char_type *get() const { return reinterpret_cast<const char_type *>(this + 1); }

		size_t ref_counter = 0;
		size_t len = 0;
		// data
	};

	inline basic_string_cursor(string_ref *_ref, size_t index) : m_index{index}, m_ref{_ref} {
		(void)_incref();
	}

	static string_ref *_alloc_ref(size_t str_size);
	static void _free_ref(string_ref *ptr);

	inline string_ref *_incref() {
		if (m_ref)
		{
			++(m_ref->ref_counter);
		}

		return m_ref;
	}

	inline void _decref() {
		if (m_ref == nullptr)
		{
			return;
		}

		// only this object holds string_ref
		if (m_ref->ref_counter <= 1)
		{
			_free_ref(m_ref);
			m_ref = nullptr;
		}
		else
		{
			--(m_ref->ref_counter);
		}
	}

	size_t m_index = 0;
	string_ref *m_ref = nullptr;
};

typedef basic_string_cursor<std::string::value_type> string_cursor;
typedef basic_string_cursor<std::wstring::value_type> wstring_cursor;
typedef basic_string_cursor<std::u8string::value_type> u8string_cursor;
typedef basic_string_cursor<std::u16string::value_type> u16string_cursor;
typedef basic_string_cursor<std::u32string::value_type> u32string_cursor;

template<typename _T>
inline basic_string_cursor<_T>::basic_string_cursor(size_t size)
	: m_index{0}, m_ref{_alloc_ref(size)} {
}

template<typename _T>
inline basic_string_cursor<_T>::basic_string_cursor(const char_type *str, size_t index)
	: m_index{index} {
	const size_t len = traits_type::length(str);
	m_ref = _alloc_ref(len);
	memcpy(m_ref->get(), str, sizeof(char_type) * len);
}

template<typename _T>
inline basic_string_cursor<_T>::~basic_string_cursor() {
	_decref();
}

template<typename _T>
inline basic_string_cursor<_T>::basic_string_cursor(const basic_string_cursor &copy)
	: m_index{copy.m_index}, m_ref{copy._incref()} {
}

template<typename _T>
inline basic_string_cursor<_T>::basic_string_cursor(basic_string_cursor &&move) noexcept
	: m_index{move.m_index}, m_ref{move.m_ref} {
	move.m_ref = nullptr;
}

template<typename _T>
inline basic_string_cursor<_T> &basic_string_cursor<_T>::operator=(const basic_string_cursor &copy) {
	if (std::addressof(copy) == this)
	{
		return *this;
	}

	_decref();

	m_index = copy.m_index;
	m_ref = copy._incref();

	return *this;
}

template<typename _T>
inline basic_string_cursor<_T> &basic_string_cursor<_T>::operator=(basic_string_cursor &&move) noexcept {
	_decref();


	m_index = move.m_index;
	m_ref = move.m_ref;

	move.m_ref = nullptr;
}

template<typename _T>
inline void basic_string_cursor<_T>::move(offset_type offset) {
	// offset will pull the index to underflow
	if (-offset > m_index)
	{
		m_index = 0;
		return;
	}

	m_index += offset;
}

template<typename _T>
inline basic_string_cursor<_T> basic_string_cursor<_T>::slice(size_t start) const {
	return basic_string_cursor(m_ref, m_index + start);
}

template<typename _T>
inline basic_string_cursor<_T> basic_string_cursor<_T>::operator+(const offset_type offset) {
	basic_string_cursor copy = *this;
	copy.move(offset);
	return copy;
}

template<typename _T>
inline basic_string_cursor<_T>::string_ref *basic_string_cursor<_T>::_alloc_ref(size_t str_size) {
	const size_t memory_size = sizeof(string_ref) + ((str_size + 1) * sizeof(char_type));
	string_ref *ptr = reinterpret_cast<string_ref *>(::operator new[](memory_size));
	new (ptr) string_ref{};
	ptr->ref_counter = 1;
	ptr->len = str_size;
	ptr->get()[str_size] = char_type();
	return ptr;
}

template<typename _T>
inline void basic_string_cursor<_T>::_free_ref(string_ref *ptr) {
	::operator delete[](reinterpret_cast<void *>(ptr));
}

namespace std
{
	template <typename _T>
	inline basic_ostream<_T> &operator<<(basic_ostream<_T> &stream,
																			 const basic_string_cursor<_T> &str_cur) {
		return stream.write(str_cur.c_str(), str_cur.size());
	}
}
