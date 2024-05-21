#pragma once
#include <iostream>
#include <cstdint>

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

	static inline ConsoleColor get_bg_color() {
		return s_bg;
	}

	static inline ConsoleColor get_fg_color() {
		return s_fg;
	}

private:
	static ConsoleColor s_bg;
	static ConsoleColor s_fg;
};
