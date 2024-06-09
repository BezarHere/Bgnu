#include "SourceTools.hpp"
#include "FilePath.hpp"

#include <regex>

struct CPreprocessor
{

	enum class Type : uint8_t
	{
		None,
		Unknown,

		Include = 4,

		Define,
		Undefine,

		If,
		Else,
		Elif,
		EndIf,

		IfDefined,
		IfNotDefined,

		Error,
		Warning,

		Pragma,
	};

	struct Token
	{
		inline Token() = default;
		Token(const IndexRange &name, const IndexRange &value);

		Type type = Type::None;
		IndexRange name;
		IndexRange value; // or anything right of the statement
		uint32_t line = 0;
	};

	static Token get_next_tk(const StrBlob &source);

	static Token _read_tk_inline(const StrBlob &source);
	static Token _parse_tk(const StrBlob &line);

	static size_t _escape(const StrBlob &source, const MutableStrBlob &destination);

};

vector<string> SourceTools::get_dependencies(const StrBlob &file) {
	return vector<string>();
}

CPreprocessor::Token CPreprocessor::get_next_tk(const StrBlob &source) {

	for (size_t i = 0; i < source.size - 1; i++)
	{
		if (StringTools::is_newline(source[i]) && !StringTools::is_newline(source[i + 1]))
		{
			return _read_tk_inline(source.slice(i + 1));
		}
	}

	return Token();
}

CPreprocessor::Token CPreprocessor::_read_tk_inline(const StrBlob &source) {
	size_t hashtag_pos = npos;

	size_t index = 0;
	for (; index < source.size; index++)
	{
		if (StringTools::is_newline(source[index]))
		{
			break;
		}

		if (source[index] == '\\')
		{
			index++;

			for (; index < source.size; index++)
			{
				if (StringTools::is_newline(source[index]))
				{
					break;
				}

				if (StringTools::is_whitespace(source[index]))
				{
					continue;
				}

				break;
			}
		}

		if (hashtag_pos == npos && source[index] == '#')
		{
			hashtag_pos = index;
		}

	}

	if (hashtag_pos == npos)
	{
		return Token();
	}
	const StrBlob line = source.slice(hashtag_pos + 1, index);
	string_char *escaped = new string_char[line.size + 1]{0};

	size_t length = _escape(line, {escaped, line.size});

	Token token = _parse_tk({escaped, length});

	delete[] escaped;

	return token;
}

CPreprocessor::Token CPreprocessor::_parse_tk(const StrBlob &line) {
	constexpr auto _is_valid_macro_name = \
		[](string_char value) { return value == '_' || isalnum(value); };

	IndexRange name = {};
	IndexRange value = {};

	for (size_t i = 0; i < line.size; i++)
	{
		if (!name.valid())
		{
			if (_is_valid_macro_name(line[i]))
			{
				size_t count = StringTools::count(
					line.data + i,
					line.size - i,
					_is_valid_macro_name
				);

				name = {i, i + count};

				i += count;
				i--;
			}

			continue;
		}

		if (!StringTools::is_whitespace(line[i]))
		{
			size_t trailing_ws = StringTools::rcount(
				line.data + i,
				line.size - i,
				StringTools::is_whitespace
			);

			value = {i, line.size - trailing_ws};
		}
	}

	return Token(name, value);
}

size_t CPreprocessor::_escape(const StrBlob &source, const MutableStrBlob &destination) {
	size_t offset = 0;
	for (size_t i = 0; i < source.size; i++)
	{
		if (source[i] == '\n' && i < source.size - 1)
		{
			// next char + advance offset
			i++;
			offset += 2;

			// skip escaped char
			if (StringTools::is_whitespace(source[i]))
			{
				continue;
			}

			// revert back (only escaped whitespace in macros)
			i--;
			offset -= 2;
		}

		if (i - offset >= destination.size)
		{
			break;
		}

		destination[i - offset] = source[i];
	}

	destination[source.size - offset] = 0;

	return source.size - offset;
}

CPreprocessor::Token::Token(const IndexRange &_name, const IndexRange &_value)
	: name{_name}, value{_value} {
	constexpr pair<const char *, Type> Names[]
	
}
