#include "Console.hpp"
#include "base.hpp"


ConsoleColor Console::s_bg = ConsoleColor::Black;
ConsoleColor Console::s_fg = ConsoleColor::White;

struct ColorResetter
{
	~ColorResetter() {
		Console::set_bg(ConsoleColor::Black);
		Console::set_fg(ConsoleColor::White);
	}
};
static ColorResetter __clr_resetter{};

struct ConsoleState
{
	ConsoleColor background;
	ConsoleColor foreground;
};


static vector<ConsoleState> g_states{};


#define ANSI_CLR_CODE(code) ("\033[" code "m")

inline void push_bg_clr_console(ConsoleColor color) {
	auto &out = std::cout;


	constexpr const char *Code[]{
		ANSI_CLR_CODE("47"),
		ANSI_CLR_CODE("47;1"),

		ANSI_CLR_CODE("40"),
		ANSI_CLR_CODE("40;1"),

		ANSI_CLR_CODE("41"),
		ANSI_CLR_CODE("41;1"),

		ANSI_CLR_CODE("42"),
		ANSI_CLR_CODE("42;1"),

		ANSI_CLR_CODE("43"),
		ANSI_CLR_CODE("43;1"),

		ANSI_CLR_CODE("44"),
		ANSI_CLR_CODE("44;1"),

		ANSI_CLR_CODE("45"),
		ANSI_CLR_CODE("45;1"),

		ANSI_CLR_CODE("46"),
		ANSI_CLR_CODE("46;1"),
	};

	out << Code[(int)color];
}

inline void push_fg_clr_console(ConsoleColor color) {
	auto &out = std::cout;


	constexpr const char *Code[]{
		ANSI_CLR_CODE("37"),
		ANSI_CLR_CODE("37;1"),

		ANSI_CLR_CODE("30"),
		ANSI_CLR_CODE("30;1"),

		ANSI_CLR_CODE("31"),
		ANSI_CLR_CODE("31;1"),

		ANSI_CLR_CODE("32"),
		ANSI_CLR_CODE("32;1"),

		ANSI_CLR_CODE("33"),
		ANSI_CLR_CODE("33;1"),

		ANSI_CLR_CODE("34"),
		ANSI_CLR_CODE("34;1"),

		ANSI_CLR_CODE("35"),
		ANSI_CLR_CODE("35;1"),

		ANSI_CLR_CODE("36"),
		ANSI_CLR_CODE("36;1"),
	};

	out << Code[(int)color];
}

void Console::set_bg(ConsoleColor color) {
	if (color == s_bg)
	{
		return;
	}

	s_bg = color;
	push_bg_clr_console(color);
}

void Console::set_fg(ConsoleColor color) {
	if (color == s_fg)
	{
		return;
	}

	s_fg = color;
	push_fg_clr_console(color);
}

void Console::push_state() {
	g_states.emplace_back();
	g_states.back().background = s_bg;
	g_states.back().foreground = s_fg;
}

void Console::pop_state() {
	const auto &old_state = g_states.back();
	g_states.pop_back();
	
	set_bg(old_state.background);
	set_fg(old_state.foreground);
}

