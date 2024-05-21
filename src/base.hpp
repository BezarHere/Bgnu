#pragma once
#include <string>
#include <vector>
#include <array>

#define HAS_FLAG(field, flag) (((field) & (flag)) == (flag))

using std::string;
using std::vector;
using std::array;

static constexpr size_t npos = (size_t)-1;

namespace inner
{
	struct EmptyCTor
	{
		template <class T>
		inline void operator()(T &val) const noexcept {
			new (&val) T();
		}
	};

	struct DTor
	{
		template <class T>
		inline void operator()(T &val) const noexcept {
			val.~T();
		}
	};

	struct TypelessCTor
	{
		inline TypelessCTor(const void *p_ptr = nullptr) : ptr{p_ptr} {}

		template <class T>
		inline void operator()(T &val) const noexcept {
			new (&val) T(*reinterpret_cast<const T *>(ptr));
		}

		const void *ptr;
	};

	struct TypelessAssign
	{
		inline TypelessAssign(const void *p_ptr = nullptr) : ptr{p_ptr} {}

		template <class T>
		inline void operator()(T &val) const noexcept {
			val = *reinterpret_cast<const T *>(ptr);
		}

		const void *ptr;
	};
}
