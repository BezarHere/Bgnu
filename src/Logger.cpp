#include "Logger.hpp"
#include <array>
#include <vector>
#include <string.h>
#include <limits>

#ifdef _DEBUG
#define IN_DEBUG true
#else
#define IN_DEBUG false
#endif

struct Logger::State
{
	using string_type = std::string;
	using string_stream_type = std::basic_ostringstream<string_type::value_type>;

	static constexpr size_t MaxIndentTemplateLength = 64;
	typedef std::array<string_type::value_type, MaxIndentTemplateLength> IndentTemplate;

	inline const string_type &get_indent_string() {
		if (_indent_dirty)
		{
			_update_indent();
		}
		return _indent_str;
	}

	inline void _update_indent() {
		_indent_dirty = false;

		if (indent > 0)
		{
			const size_t template_len = strnlen(_indent_template.data(), _indent_template.size());
			_indent_str.resize(template_len * indent);
			for (size_t i = 0; i < indent; i++)
			{
				string_type::traits_type::copy(
					_indent_str.data() + (template_len * i),
					_indent_template.data(),
					template_len
				);
			}
		}
		else
		{
			_indent_str.clear();
		}
	}

	uint8_t indent = 0;
	bool _indent_dirty = true;
	IndentTemplate _indent_template = {"  "};
	string_type _indent_str;

	static std::vector<Logger::State> s_stack;
};

std::vector<Logger::State> Logger::State::s_stack;

Logger::State Logger::s_state = {};

bool Logger::s_debug = IN_DEBUG;
bool Logger::s_verbose = false;
std::mutex Logger::s_log_mutex = {};

void Logger::_push_state() {
	State::s_stack.push_back(s_state);
}

void Logger::_pop_state() {
	s_state = State::s_stack.back();
	State::s_stack.pop_back();
}

void Logger::_write_indent(std::ostream &stream) {
	stream << s_state.get_indent_string();
}

void Logger::_write_indent(FILE *pfile) {
	fputs(s_state.get_indent_string().c_str(), pfile);
}

const char *Logger::_get_indent_str() {
	return s_state.get_indent_string().c_str();
}

void Logger::raise_indent() {
	typedef std::numeric_limits<decltype(State::indent)> IndentLimits;

	if (s_state.indent == IndentLimits::max())
	{
		return;
	}

	s_state.indent++;
	s_state._indent_dirty = true;
}

void Logger::lower_indent() {

	if (!s_state.indent)
	{
		return;
	}

	s_state.indent--;
	s_state._indent_dirty = true;
}
