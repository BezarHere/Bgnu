#include "CPreprocessor.hpp"


constexpr CPreprocessor::Type CPreprocessor::_get_tk_type(const StrBlob &name) {
	for (const auto &p : TypeNamePairs)
	{
		if (StringTools::equal(name.data, p.first, name.length()))
		{
			return p.second;
		}
	}

	return Type::Unknown;
}

void CPreprocessor::gather_all_tks(const StrBlob &source, vector<Token> &output) {
	size_t index = 0;
	while (index < source.size)
	{
		Token token = get_next_tk(source.slice(index));

		size_t end = std::max(token.name.end, token.value.end);

		if (token.type == Type::None)
		{
			break;
		}

		index += end + 1;

		output.push_back(token);
	}
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

string CPreprocessor::get_include_path(const StrBlob &value) {
	StrBlob trimmed = StringTools::trim(
		value,
		[](string_char value) {
			return StringTools::is_whitespace(value) || value == '"' || value == '<' || value == '>';
		}
	);
	return string(trimmed.begin(), trimmed.length());
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
			break;
		}
	}

	return Token(line.data, name, value);
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

CPreprocessor::Token::Token(const string_char *source, const IndexRange &_name, const IndexRange &_value)
	: type{_get_tk_type(_name.to_blob(source))},
	name{_name}, value{_value} {



}
