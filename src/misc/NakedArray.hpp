#pragma once
#include <array>
#include <stdint.h>

template <typename _T, size_t _Sz>
struct NakedArray
{
public:
	inline constexpr NakedArray() = default;

	inline constexpr operator _T *() { return reinterpret_cast<_T *>(m_data); }
	inline constexpr operator const _T *() const { return reinterpret_cast<const _T *>(m_data); }

	inline constexpr _T &operator *() { return *reinterpret_cast<_T *>(m_data); }
	inline constexpr const _T &operator *() const { return *reinterpret_cast<const _T *>(m_data); }

	inline constexpr _T &operator[](const size_t pos) {
		return ((_T *)m_data)[pos];
	}

	inline constexpr const _T &operator[](const size_t pos) const {
		return ((const _T *)m_data)[pos];
	}

private:
	typedef uint8_t element_type;

	static constexpr size_t BytesSize = sizeof(_T[_Sz]);
	static constexpr size_t ElmCount =
		BytesSize / sizeof(element_type) + size_t(BytesSize % sizeof(element_type) > 0);

	element_type m_data[ElmCount];
};

namespace std
{
	template <typename _T, size_t N>
	inline constexpr size_t size(const NakedArray<_T, N> &_arr)
	{
		(void)_arr;
		return N;
	}
}
