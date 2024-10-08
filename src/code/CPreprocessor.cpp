#include "CPreprocessor.hpp"
#include "CodeTokenizer.hpp"


constexpr CPreprocessor::Type CPreprocessor::_get_tk_type(const StrBlob &name) {
	for (const auto &p : TypeNamePairs)
	{
		if (string_tools::equal(name.data, p.first, name.length()))
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

		// std::cout << "token n1: \"" << string(token.name.begin + source.data, token.name.length()) << '"' << '\n';
		// std::cout << "token v1: \"" << string(token.value.begin + source.data, token.value.length()) << '"' << '\n';

		// offset token to the entire source (NOTE THAT: end is calculated before shifting the token)
		token.name = token.name.shifted(index);
		token.value = token.value.shifted(index);

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
		if ((string_tools::is_newline(source[i]) || i == 0) && !string_tools::is_newline(source[i + 1]))
		{

			const size_t read_start = i + (i ? 1 : 0);

			// + 1 to i if it's not zero
			Token token = _read_tk_inline(source.slice(read_start));
			token.name = token.name.shifted(read_start);
			token.value = token.value.shifted(read_start);

			/*
			std::cout << "TOK PARSED N: " << token.name
				<< " \"" << string(source.data + token.name.begin, token.name.length()) << "\"\n";

			std::cout << "TOK PARSED V: " << token.value
				<< " \"" << string(source.data + token.value.begin, token.value.length()) << "\"\n";
			*/
			return token;
		}
	}

	return Token();
}

string CPreprocessor::get_include_path(const StrBlob &value) {
	constexpr auto is_opening = [](string_char value) {return value == '"' || value == '<';};
	constexpr auto is_closing = [](string_char value) {return value == '"' || value == '>';};

	// pos of the opening quote/bracket
	size_t opening = CodeTokenizer::match_unescaped(
		value,
		is_opening
	);

	if (opening == npos)
	{
		return "";
	}

	opening = opening + 1;

	// pos of the closing quote/bracket
	const size_t closing = CodeTokenizer::match_unescaped(
		value.slice(opening),
		is_closing
	) + opening;

	return string(value.data + opening, closing - opening);
}

CPreprocessor::Token CPreprocessor::_read_tk_inline(const StrBlob &source) {
	size_t hashtag_pos = npos;

	size_t index = 0;
	for (; index < source.size; index++)
	{
		if (string_tools::is_newline(source[index]))
		{
			break;
		}

		if (source[index] == '\\')
		{
			index++;

			for (; index < source.size; index++)
			{
				if (string_tools::is_newline(source[index]))
				{
					break;
				}

				if (string_tools::is_whitespace(source[index]))
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
	token.name = token.name.shifted(hashtag_pos + 1);
	token.value = token.value.shifted(hashtag_pos + 1);

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
				size_t count = string_tools::count(
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

		if (!string_tools::is_whitespace(line[i]))
		{
			size_t trailing_ws = string_tools::rcount(
				line.data + i,
				line.size - i,
				string_tools::is_whitespace
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
			if (string_tools::is_whitespace(source[i]))
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
