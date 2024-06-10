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
	inline constexpr Blob(std::nullptr_t) noexcept {}

	inline constexpr Blob(value_type *_data, size_type _size) noexcept : size{_size}, data{_data} {}

	inline constexpr Blob(value_type *begin, value_type *end)
		: size{static_cast<size_t>(end - begin)}, data{begin} {
		if (end < begin)
		{
			throw std::range_error("end < begin");
		}
	}

	template <size_type N>
	inline constexpr Blob(value_type(&memory)[N]) noexcept : size{N}, data{memory} {}

	// blob does not own the memory, so we can directly use it for container types
	template <class Cnt, std::enable_if_t<!std::is_void_v<Cnt::size>>>
	inline constexpr Blob(const Cnt &container) noexcept
		: size{container.size()}, data{container.data()} {
	}

	inline value_type *begin() const noexcept { return data; }
	inline value_type *end() const noexcept { return data + size; }

	// returns a slice from start to the end of the blob
	inline Blob slice(size_type start) const {
		if (start >= size)
		{
			throw std::range_error(
				"'start' should be contained in [0, size)"
			);
		}
		return {data + start, data + size};
	}

	// returns a slice from start to end (exclusive end)
	inline Blob slice(size_type start, size_type end) const {
		if (start >= size || end > size || end < start)
		{
			throw std::range_error(
				"'start' and 'end' should be contained in [0, size), with 'start' <= 'end'"
			);
		}
		return {data + start, data + end};
	}

	inline size_t length() const noexcept { return size; }
	inline bool empty() const noexcept { return size == 0 || data == nullptr; }


	// we don't handle out-of-range errors, this is a blob
	inline value_type &operator[](const size_type index) const { return data[index]; }

	size_type size = 0;
	value_type *data = nullptr;
};
