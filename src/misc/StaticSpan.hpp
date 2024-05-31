#pragma once
#include <array>
#include <new>
#include "Container.hpp"
#include "../Logger.hpp"

template <typename _T, std::size_t _Size>
class StaticSpan
{
public:
	using value_type = _T;
	using size_type = uint32_t;
	static constexpr std::size_t max_size = _Size;


private:
	inline void _Dtor(size_type index);
public:

	inline size_t size() const { return m_cursor; }
	inline size_t capacity() const { return max_size; }

	inline bool empty() const { return m_cursor; }
	inline bool full() const { return size() == capacity(); }

	inline value_type &operator[](size_type index) {
#if _DEBUG_CONTAINER_LEVEL > 0
		LOG_ASSERT_V(index < size(), "index [%llu] should be bound in [0, %llu)", index, size());
#endif
		return *reinterpret_cast<value_type *>(m_block.data());
	}

	inline const value_type &operator[](size_type index) const {
#if _DEBUG_CONTAINER_LEVEL > 0
		LOG_ASSERT_V(index < size(), "index [%llu] should be bound in [0, %llu)", index, size());
#endif
		return *reinterpret_cast<const value_type *>(m_block.data());
	}

	inline value_type *begin() { return reinterpret_cast<value_type *>(m_block.data()); }
	inline value_type *end() { return begin() + m_cursor; }

	inline const value_type *begin() const { return reinterpret_cast<const value_type *>(m_block.data()); }
	inline const value_type *end() const { return begin() + m_cursor; }

	inline value_type &push_back(const value_type &value) {
		if (full())
		{
			throw std::length_error("push_back() on a full static span");
		}

		value_type *vptr = end();
		new(vptr) value_type(value);
		m_cursor++;
		return *vptr;
	}

	template <typename ..._Args>
	inline value_type &emplace_back(_Args &&...args) {
		if (full())
		{
			throw std::length_error("push_back() on a full static span");
		}

		value_type *vptr = end();
		new(vptr) value_type(std::forward<_Args>(args)...);
		m_cursor++;
		return *vptr;
	}

	inline value_type &back() {
#if _DEBUG_CONTAINER_LEVEL > 0
		if (empty())
		{
			throw std::runtime_error("back() on empty static span");
		}
#endif
		return *(end() - 1);
	}

	inline const value_type &back() const {
#if _DEBUG_CONTAINER_LEVEL > 0
		if (empty())
		{
			throw std::runtime_error("back() on empty static span");
		}
#endif
		return *(end() - 1);
	}

	inline void pop_back() {
		m_cursor--;
		_Dtor(m_cursor);
	}

private:
	typedef char memory_element;
	typedef memory_element memory_object[sizeof(value_type)];
	typedef std::array<memory_object, max_size> memory_block;

	size_type m_cursor;
	memory_block m_block;
};

template<typename _T, std::size_t _Size>
inline void StaticSpan<_T, _Size>::_Dtor(size_type index) {
	if constexpr (!std::is_trivially_destructible_v<_T>)
	{
		begin()[index].~_T();
	}
}
