#include "Logger.hpp"
#include <array>
#include <string.h>

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

		// hopefully, str() will only use chars until the put pos, otherwise we are screwed
		_indent_template_stream.seekp(0, std::ios::seekdir::_S_beg);

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

			_indent_str = _indent_template_stream.str();
		}
		else
		{
			_indent_str.clear();
		}
	}

	size_t indent = 0;
	bool _indent_dirty = true;
	IndentTemplate _indent_template = {"  "};
	string_stream_type _indent_template_stream;
	string_type _indent_str;
};

Logger::State Logger::s_state = {};
bool Logger::s_verbose = false;


void Logger::_write_indent(std::ostream &stream) {
	stream << s_state.get_indent_string();
}