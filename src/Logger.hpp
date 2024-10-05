#pragma once
#include "base.hpp"
#include "Console.hpp"
#include "misc/Error.hpp"

#include <mutex>

#define LOG_ASSERT(condition) if (!(condition)) Logger::_assert_fail(#condition, nullptr)
#define LOG_ASSERT_V(condition, ...) \
	if (!(condition)) Logger::_assert_fail(#condition, __VA_ARGS__)

class Logger : public Console
{
	friend class Startup;
public:

	static void _push_state();
	static void _pop_state();

	static void _write_indent(std::ostream &stream);
	static void _write_indent(FILE *pfile);
	static const char *_get_indent_str();

	static void raise_indent();
	static void lower_indent();

	static inline bool is_debug() { return s_debug; }
	static inline bool is_verbose() { return s_verbose; }

	template <typename... _Args>
	inline static void log(_Args &&...args) {
		std::scoped_lock<std::mutex> lock{s_log_mutex};
		
		_write_indent(std::cout);
		_format_join(std::cout, std::forward<_Args>(args)...);
		std::cout << '\n';
	}

	// only indented, no trailing new line or defined-coloring
	inline static void write_raw(const char *format, ...) {
		std::scoped_lock<std::mutex> lock{s_log_mutex};

		Console::push_state();
		_write_indent(stdout);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		Console::pop_state();
	}

	// to be used with the LOG_ASSERT/LOG_ASSERT_V macro
	// `format` can be null to display the assertion fail without any message
	NORETURN static inline
		void _assert_fail(const char *assert_cond, const char *format, ...) {
		std::scoped_lock<std::mutex> lock{s_log_mutex};

		Console::push_state();
		Console::set_bg(ConsoleColor::Black);
		Console::set_fg(ConsoleColor::IntenseMagenta);

		fprintf(stderr, "CONDITION \"%s\" FAILED\n", assert_cond);

		if (format)
		{
			_write_indent(stderr);


			va_list valist{};
			va_start(valist, format);
			vfprintf(stderr, format, valist);
			va_end(valist);

			fputc('\n', stderr);
		}

		fflush(stderr);

		Console::pop_state();

		throw std::runtime_error(assert_cond);
	}

	static inline void error(const char *format, ...) {
		std::scoped_lock<std::mutex> lock{s_log_mutex};

		Console::push_state();
		Console::set_bg(ConsoleColor::Black);
		Console::set_fg(ConsoleColor::IntenseRed);

		_write_indent(stderr);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stderr, format, valist);
		va_end(valist);

		fputc('\n', stderr);

		Console::pop_state();
	}

	static inline void error(const ErrorReport &report) {
		return error("ERR[%d]: %s", (int)report.code, report.message.c_str());
	}

	static inline void warning(const char *format, ...) {
		std::scoped_lock<std::mutex> lock{s_log_mutex};

		Console::push_state();
		Console::set_bg(ConsoleColor::Black);
		Console::set_fg(ConsoleColor::Yellow);

		_write_indent(stdout);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		fputc('\n', stdout);

		Console::pop_state();
	}

	static inline void notify(const char *format, ...) {
		std::scoped_lock<std::mutex> lock{s_log_mutex};

		Console::push_state();
		Console::set_bg(ConsoleColor::Black);
		Console::set_fg(ConsoleColor::Green);

		_write_indent(stdout);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		fputc('\n', stdout);

		Console::pop_state();
	}

	static inline void note(const char *format, ...) {
		std::scoped_lock<std::mutex> lock{s_log_mutex};

		Console::push_state();
		Console::set_bg(ConsoleColor::Black);
		Console::set_fg(ConsoleColor::Cyan);

		_write_indent(stdout);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		fputc('\n', stdout);

		Console::pop_state();
	}

	// logs the message in a gray color if the logger is in debug or verbose mode
	static inline void debug(const char *format, ...) {
		if (!(s_debug || s_verbose))
		{
			return;
		}
		std::scoped_lock<std::mutex> lock{s_log_mutex};

		Console::push_state();
		Console::set_bg(ConsoleColor::Black);
		// hopefully this is just dark gray for foreground
		Console::set_fg(ConsoleColor::IntenseBlack);

		_write_indent(stdout);

		// if we are not in debug (printing because of verbose mode)
		if (!s_debug)
		{
			fputs("DBG: ", stdout);
		}

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		fputc('\n', stdout);

		Console::pop_state();
	}

	// logs the message in a gray color if the logger is in verbose mode
	static inline void verbose(const char *format, ...) {
		if (!s_verbose)
		{
			return;
		}
		std::scoped_lock<std::mutex> lock{s_log_mutex};

		Console::push_state();
		Console::set_bg(ConsoleColor::Black);
		// hopefully this is just dark gray for foreground
		Console::set_fg(ConsoleColor::IntenseBlack);

		_write_indent(stdout);

		fputs("VRB: ", stdout);

		va_list valist{};
		va_start(valist, format);
		vfprintf(stdout, format, valist);
		va_end(valist);

		fputc('\n', stdout);

		Console::pop_state();
	}

private:
	static inline void _make_verbose() {
		s_verbose = true;
	}

private:
	struct State;
	static State s_state;
	static bool s_debug; // default = defined(_DEBUG)
	static bool s_verbose; // default = false
	static std::mutex s_log_mutex;
};
