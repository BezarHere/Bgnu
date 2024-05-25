#pragma once
#include <iostream>
#include <sstream>
#include <cstdint>
#include <stdarg.h>

// Intense: Bright or Bold (implementation dependent)
enum class ConsoleColor : uint8_t
{
	White,
	IntenseWhite,

	// Gray on most implementation
	Black,
	// Gray on most implementation
	IntenseBlack,

	Red,
	IntenseRed,

	Green,
	IntenseGreen,

	Yellow,
	IntenseYellow,

	Blue,
	IntenseBlue,

	Magenta,
	IntenseMagenta,

	Cyan,
	IntenseCyan,
};

class Console
{
public:

	static void set_bg(ConsoleColor color);
	static void set_fg(ConsoleColor color);
	static void push_state();
	static void pop_state();

	static inline ConsoleColor get_bg_color() {
		return s_bg;
	}

	static inline ConsoleColor get_fg_color() {
		return s_fg;
	}

	static inline void warning(const char *format, ...) {
		push_state();
		set_bg(ConsoleColor::Black);
		set_fg(ConsoleColor::Yellow);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		pop_state();
	}

	static inline void error(const char *format, ...) {
		push_state();
		set_bg(ConsoleColor::Black);
		set_fg(ConsoleColor::IntenseRed);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		pop_state();
	}

	static inline void debug(const char *format, ...) {
		push_state();
		set_bg(ConsoleColor::Black);
		// hopefully this is just dark gray for foreground
		set_fg(ConsoleColor::IntenseBlack);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		pop_state();
	}

private:
	static ConsoleColor s_bg;
	static ConsoleColor s_fg;
};

// for empty args
static inline void _format_join(std::ostream &stream) {
	(void)stream;
}

template <typename T>
static inline void _format_join(std::ostream &stream, const T &value) {
	stream << value;
}

template <typename T1, typename T2, typename... VArgs>
static inline void _format_join(std::ostream &stream, const T1 &value1, const T2 &value2, const VArgs &... args) {
	stream << value1;
	return _format_join(stream, value2, args...);
}

template <typename... VArgs>
static inline std::string format_join(const VArgs &... args) {
	std::ostringstream ss{};
	_format_join(ss, args...);
	return ss.str();
}

