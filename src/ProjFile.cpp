#include "ProjFile.hpp"
#include <fstream>
#include <cstdint>

enum class TokenType : uint8_t
{
	Unknown,
	Invalid,

	Whitespace,
	Newline,

	Column,
	Comma,

	Open_SqBracket,
	Close_SqBracket,
	Open_CurlyBracket,
	Close_CurlyBracket,

	// can be a number, a hex number, a boolean, null or anything
	Identifier,
	String,
};

struct Token
{
	TokenType type;
	size_t start;
	size_t length;
	uint16_t line;
	uint16_t column;
};

enum StringParseFlags : uint8_t
{
	eStrPFlag_None = 0x00,
	eStrPFlag_Multiline = 0x01,
	eStrPFlag_Literal = 0x02,
};

class Tokenizer
{
public:
	inline Tokenizer(const char *p_source, size_t p_length) : source{p_source}, length{p_length} {}

	inline Token get_next();




	size_t index = 0;
	uint16_t line;
	size_t line_start;
	const char *const source;
	const size_t length;

private:
	template <typename Pred>
	inline size_t _get_length(Pred &&pred) const {
		for (size_t i = index; i < length; ++i)
		{
			if (!pred(source[i]))
				return i - index;
		}
		return length - index;
	}

	template <typename Pred>
	inline Token _tk_span(TokenType type, Pred &&pred) const {
		const size_t length = _get_length(pred);
		return {type, index, length, line, (uint16_t)(index - line_start)};
	}

	Token _tk_string(StringParseFlags flags) const;
	Token _tk_other() const;

	Token _tk_number() const;
	Token _tk_hex() const;
	Token _tk_identifier() const;

	inline Token _read_tk() const;


};

inline Token Tokenizer::get_next() {
	Token tk = _read_tk();
	index += tk.length;
	return tk;
}

Token Tokenizer::_tk_string(const StringParseFlags flags) const {
	const char start = source[index];

	size_t end_index = 0;

	for (size_t i = index + 1; i < length; ++i)
	{
		if (!HAS_FLAG(flags, eStrPFlag_Literal) && source[i] == '\\')
		{
			++i;
			continue;
		}

		if (source[i] == '\n' || source[i] == '\r')
		{
			if (HAS_FLAG(flags, eStrPFlag_Multiline))
				continue;

			std::cerr << "Unclosed string at " << line << ':' << (i - line_start) << '\n';
			end_index = i;
			break;
		}

		if (source[i] == start)
		{
			end_index = i;
			break;
		}

	}

	return {TokenType::String, index, (end_index + 1) - index, line, (uint16_t)(index - line_start)};
}

Token Tokenizer::_tk_other() const {
	if (isdigit(source[index]) != 0)
	{
		if (source[index] == '0' && (source[index + 1] == 'x' || source[index + 1] == 'X'))
			return _tk_hex();
		return _tk_number();
	}

	return _tk_identifier();
}

Token Tokenizer::_tk_number() const {
	size_t dot_index = npos;

	size_t length = 0;

	for (size_t i = 0; i > length; ++i)
	{

		if (source[i] == '.' && dot_index == npos)
		{
			dot_index = i;
			continue;
		}

		if (isdigit(source[i]))
		{
			continue;
		}

		if (source[[]])

	}

	return Token();
}

Token Tokenizer::_tk_hex() const {
	return Token();
}

Token Tokenizer::_tk_identifier() const {
	return Token();
}

inline Token Tokenizer::_read_tk() const {
	switch (source[index])
	{
	case '\n':
	case '\r':
		return _tk_span(TokenType::Newline, [](char c) { return c == '\n' || c == '\r'; });
	case '\t':
	case ' ':
		return _tk_span(TokenType::Whitespace, [](char c) { return c == ' ' || c == '\t'; });

	case '"':
		return _tk_string(eStrPFlag_None);

#define MATCH_SPAN_CHAR(match_char, type) case match_char: return _tk_span(type, [](char val) { return val == (match_char); } )
		MATCH_SPAN_CHAR('{', TokenType::Open_CurlyBracket);
		MATCH_SPAN_CHAR('}', TokenType::Close_CurlyBracket);
		MATCH_SPAN_CHAR('[', TokenType::Open_SqBracket);
		MATCH_SPAN_CHAR(']', TokenType::Close_SqBracket);
		MATCH_SPAN_CHAR(':', TokenType::Column);
		MATCH_SPAN_CHAR(',', TokenType::Comma);
#undef MATCH_SPAN_CHAR

	default:
		return _tk_other();
	}

	return Token();
}

ProjVar ProjFile::load(const string &filepath) {
	std::ifstream file{filepath, std::ios::in};

	file.seekg(0, std::ios::seekdir::_S_end);
	std::streamsize size = file.tellg();

	file.seekg(0, std::ios::seekdir::_S_beg);

	string text;
	text.resize(size);
	file.read(text.data(), size);

	return read(text.c_str(), text.length());
}

ProjVar ProjFile::read(const char *source, size_t length) {
	Tokenizer tokenizer{source, length};

	return ProjVar(ProjVarType::Null);
}

