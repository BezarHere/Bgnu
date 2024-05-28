#pragma once
#include <stdexcept>

/// @brief holds a pointer and a size of a region in memory,
/// @brief can be used to pass static heap arrays
/// @note DOES NOT OWN ANYTHING, it will NEVER dispose of the memory it's pointing to
template <typename _T>
struct Blob final
{
	using value_type = _T;
	using size_type = size_t;

	inline constexpr Blob() noexcept = default;
	inline constexpr Blob(std::nullptr_t) noexcept = default;

	inline constexpr Blob(value_type *_data, size_type _size) noexcept : size{_size}, data{_data} {}

	inline constexpr Blob(value_type *begin, value_type *end) : size{end - begin}, data{begin} {
		if (end < begin)
		{
			throw std::range_error("end < begin");
		}
	}

	template <size_type N>
	inline constexpr Blob(value_type memory[N]) noexcept : size{N}, data{memory} {}

	// blob does not own the memory, so we can directly use it for container types
	template <class Cnt>
	inline constexpr Blob(const Cnt &container) noexcept
		: size{container.size()}, data{container.data()} {
	}

	inline value_type *begin() const { return data; }
	inline value_type *end() const { return data + size; }

	// we don't handle out-of-range errors, this is a blob
	inline value_type &operator[](const size_type index) const { return data[index]; }

	size_type size = 0;
	value_type *data = nullptr;
};
