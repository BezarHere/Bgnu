#pragma once
#include "Console.hpp"

class Logger : public Console
{
public:

	static void _write_indent(std::ostream &stream);

	template <typename... _Args>
	inline static void log(_Args &&...args) {
		_write_indent(std::cout);
		_format_join(std::cout, std::forward<_Args>(args)...);
	}

private:
	struct State;
	static State s_state;
	static bool s_verbose;
};
