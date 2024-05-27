#pragma once
#include "base.hpp"

/// @brief inclusive begin, exclusive end range
/// @tparam _T the range value type
template <typename _T>
struct TRange
{
	static_assert(std::is_arithmetic_v<_T> || std::is_pointer_v<_T>, "only arithmetic or pointer ranges are allowed");
	using iterable = TRangeIterable<_T>;

	inline constexpr TRange() = default;

	inline constexpr TRange(_T pstart, _T pend) : begin{pstart}, end{pend} {
	}

	inline constexpr _T length() const {
		return end - begin;
	}

	inline constexpr bool valid() const {
		return end > begin;
	}

	inline constexpr bool contains(const _T value) const {
		return value < end && value >= begin;
	}

	_T begin{};
	_T end{};
};

using FloatRange = TRange<float>;
using IntRange = TRange<int>;
using IndexRange = TRange<size_t>;

