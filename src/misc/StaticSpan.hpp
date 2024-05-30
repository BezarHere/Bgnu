#pragma once
#include <array>
#include <new>

template <typename _T, std::size_t _Size>
class StaticSpan
{
public:
	using value_type = _T;
	static constexpr std::size_t max_size = _Size;


private:
	typedef char memory_element;
	typedef memory_element memory_object[sizeof(value_type)];
	typedef std::array<memory_object, max_size> memory_block;

	size_t m_cursor;
	memory_block m_block;
};
