#pragma once
#include <string>
#include <stack>
#include <vector>
#include <array>
#include <cstdint>

#include <map>

#include <limits>

#include "misc/Blob.hpp"

#define HAS_FLAG(field, flag) (((field) & (flag)) == (flag))

template <typename _T1, typename _T2 = _T1>
using pair = std::pair<_T1, _T2>;

using std::vector;
using std::array;

template <typename T>
using vector_stack = std::stack<T, std::vector<T>>;

typedef std::string string;
typedef string::value_type string_char;

using std::streamsize;
typedef uint64_t hash_t;

// mutable blob of string_chars
typedef Blob<string_char> MutableStrBlob;
// immutable blob of string_chars (const string_char)
typedef Blob<const string_char> StrBlob;
// mutable blob of string_chars that are expected to be modified externally
typedef Blob<volatile string_char> VolatileStrBlob;
// immutable blob of string_chars that are expected to be modified externally but not internally
typedef Blob<const volatile string_char> CVolatileStrBlob;

static constexpr size_t npos = (size_t)-1;
static constexpr streamsize streamsize_max = std::numeric_limits<streamsize>::max();


#ifdef _MSC_VER
#define NODISCARD _NODISCARD
#define NORETURN __declspec(noreturn)
#define ALWAYS_INLINE inline __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#define NODISCARD [[__nodiscard__]]
#define NORETURN __attribute__ ((__noreturn__))
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#endif

static ALWAYS_INLINE constexpr const char *to_cstr(const char *value) { return value; }
template <typename _T>
static ALWAYS_INLINE constexpr const char *to_cstr(const Blob<_T> &value) { return value.data; }
static ALWAYS_INLINE const char *to_cstr(const string &value) { return value.c_str(); }

ALWAYS_INLINE constexpr const char *to_boolalpha(const bool value) {
	return value ? "true" : "false";
}

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

	template <typename T>
	struct EqualTo
	{
		inline EqualTo(T &&_value) : value{_value} {}

		template <typename E>
		inline bool operator()(E &&other) const { return other == value; }

		template <typename E>
		inline bool operator()(const E &other) const { return other == value; }

		T value;
	};

	template <typename Op>
	struct InvertOp
	{
		inline InvertOp(Op &&_op) : _operator{_op} {}

		template <typename T>
		inline bool operator()(T &&arg) const { return !_operator(arg); }

		Op _operator;
	};

}
