#pragma once
#include "base.hpp"


/// @brief inclusive start, exclusive end range
/// @tparam _T the range value type
template <typename _T>
struct TRange
{
	static_assert(std::is_arithmetic_v<_T> || std::is_pointer_v<_T>, "only arithmetic or pointer ranges are allowed");

	inline constexpr TRange(_T pstart, _T pend) : start{pstart}, end{pend} {
	}

	inline constexpr _T length() const {
		return end - start;
	}

	inline constexpr bool valid() const {
		return end > start;
	}

	inline constexpr bool contains(const _T value) const {
		return value < end && value >= start;
	}

	_T start, end;
};

using FloatRange = TRange<float>;
using IntRange = TRange<int>;
using IndexRange = TRange<size_t>;

