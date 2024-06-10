#pragma once
#include "base.hpp"

/// @brief inclusive begin, exclusive end range
/// @tparam _T the range value type
template <typename _T>
struct TRange
{
	static_assert(std::is_arithmetic_v<_T> || std::is_pointer_v<_T>, "only arithmetic or pointer ranges are allowed");

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

	// returns a copy expanded by `value`.
	// expansion size is `2 * value` (e.g. `{1, 5}.expanded(2)` == `{-1, 7}`)
	inline constexpr TRange expended(const _T value) const {
		return {begin - value, end + value};
	}

	// returns a copy shifted by `value` (e.g. `{2, 7}.shifted(-3)` == `{-1, 4}`)
	inline constexpr TRange shifted(const _T value) const {
		return {begin + value, end + value};
	}

	// constructs a blob ranging from `base + begin` to `base + end`
	template <typename _V>
	ALWAYS_INLINE constexpr Blob<_V> to_blob(_V *base) const noexcept {
		return {base + begin, base + end};
	}

	_T begin{};
	_T end{};
};

using FloatRange = TRange<float>;
using IntRange = TRange<int>;
using IndexRange = TRange<size_t>;

