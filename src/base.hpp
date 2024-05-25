#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdint>

#define HAS_FLAG(field, flag) (((field) & (flag)) == (flag))

using std::string;
using std::vector;
using std::array;


static constexpr size_t npos = (size_t)-1;


#ifdef _MSC_VER
#define NODISCARD _NODISCARD
#define NORETURN __declspec(noreturn)
#define ALWAYS_INLINE inline __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define NODISCARD [[__nodiscard__]]
#define NORETURN __attribute__ ((__noreturn__))
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#endif



namespace inner
{
	static constexpr size_t ExceptionBufferSz = 1024;

	// a struct that can be called with any arguments, but it does not do anything
	struct DryCallable
	{

		template <typename... _Args>
		inline void operator()(_Args &&...) const noexcept {
		}

	};

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

	template <typename T, typename NameType = const char *>
	struct NamedValue
	{
		NameType name;
		T value;
	};
}
