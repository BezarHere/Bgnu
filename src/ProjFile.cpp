#include "ProjFile.hpp"
#include <fstream>
#include <string.h>

enum class TokenType : uint8_t
{
	Unknown,
	Invalid,
	Eof,

	Whitespace,
	Newline,

	Column,
	Comma,

	Open_SqBracket,
	Close_SqBracket,
	Open_CurlyBracket,
	Close_CurlyBracket,

	// can be a number, a boolean, null or anything
	Identifier,
	String,
};

struct TKPos
{
	inline TKPos() {}
	inline TKPos(uint16_t p_line, uint16_t p_column) : line{p_line}, column{p_column} {}

	uint16_t line = 0, column = 0;
};

struct Token
{
	TokenType type;

	const char *str;
	size_t length;

	TKPos pos;
};

enum StringParseFlags : uint8_t
{
	eStrPFlag_None = 0x00,
	eStrPFlag_Multiline = 0x01,
	eStrPFlag_Literal = 0x02,
};

inline static bool is_identifier_char(char c) {
	return isalnum(c) || c == '_' || c == '.';
}

inline static bool is_number_identifier(const char *str, const size_t len) {
	(void)len;
	return isdigit(*str);
}

#pragma region(tokenizer)
class Tokenizer
{
public:
	inline Tokenizer(const char *p_source, size_t p_length) : source{p_source}, length{p_length} {}

	inline Token get_next();

	inline void put_all(vector<Token> &output) {
		const size_t old_index = index;

		while (index < length)
		{
			output.emplace_back(get_next());
			if (output.back().type == TokenType::Eof)
				break;
		}

		index = old_index;
	}

	inline TKPos get_pos() const noexcept { return {line, uint16_t(index - line_start)}; }

	size_t index = 0;
	uint16_t line = 0;
	size_t line_start = 0;
	const char *const source;
	const size_t length;

private:
	inline const char *_get_current_str() const { return source + index; }

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
		return {type, _get_current_str(), length, get_pos()};
	}

	Token _tk_string(StringParseFlags flags) const;
	Token _tk_other() const;

	Token _tk_number() const;
	Token _tk_hex() const;
	Token _tk_identifier() const;

	inline Token _read_tk() const;


};

inline Token Tokenizer::get_next() {
	Token token = _read_tk();

	if (token.length == 0)
	{
		//? should be an error here?
		++index;
	}
	index += token.length;

	if (token.type == TokenType::Newline)
	{
		line += token.length;
		line_start = index;
	}

	return token;
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

	return {TokenType::String, _get_current_str(), (end_index + 1) - index, get_pos()};
}

Token Tokenizer::_tk_identifier() const {
	if (!is_identifier_char(source[index]))
		return {TokenType::Unknown, _get_current_str(), 1, get_pos()};

	size_t result_end = length;

	for (size_t i = index; i < length; ++i)
	{
		if (is_identifier_char(source[i]))
			continue;

		result_end = i;
		break;
	}

	return {TokenType::Identifier, _get_current_str(), result_end - index, get_pos()};
}

inline Token Tokenizer::_read_tk() const {
	std::cout << index << ' ' << length << '\n';
	if (index >= length)
	{
		return {TokenType::Eof, _get_current_str(), 1, get_pos()};
	}

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
		return _tk_identifier();
	}

	return Token();
}

#pragma endregion

#pragma region(parser)

class Parser
{
public:
	inline Parser(const char *source, const size_t source_length,
								const Token *tks, const size_t count)
		: m_source{source}, m_source_length{source_length}, m_tokens{tks}, m_size{count} {
	}

	ProjVar::VarDict parse();

	static bool is_useless_token(const Token &token) {
		if (token.length == 0)
		{
			return true;
		}

		return token.type == TokenType::Whitespace || token.type == TokenType::Unknown;
	}

	inline const Token *get_ptr() const noexcept { return &m_tokens[current_index]; }
	inline const Token &get_current() const noexcept { return m_tokens[current_index]; }

	inline size_t size() const noexcept { return m_size; }
	inline const Token *data() const noexcept { return m_tokens; }

	size_t current_index;
private:
	void _skip_useless();

	ProjVar _get_var();
	std::pair<ProjVar::VarString, ProjVar> _get_kv();


	ProjVar _parse_var_string(bool trim);
	ProjVar _parse_var_identifier();
	ProjVar _parse_var_number();

	ProjVar _parse_var_simple();
	ProjVar _parse_var_dict();
	ProjVar _parse_var_array();


private:
	const char *m_source;
	const size_t m_source_length;
	const Token *m_tokens;
	const size_t m_size;
};

void Parser::_skip_useless() {
	while (current_index < m_size && Parser::is_useless_token(m_tokens[current_index]))
	{
		++current_index;
	}
}


ProjVar Parser::_parse_var_string(bool trim) {
	return ProjVar();
}

ProjVar Parser::_parse_var_identifier() {
	const char *token_str = m_source + get_current().start;
	constexpr const char null_str[] = "null";
	constexpr const char true_str[] = "true";
	constexpr const char false_str[] = "false";

	if (strncmp(token_str, null_str, std::size(null_str) - 1) == 0)
		return ProjVar(ProjVarType::Null);

	if (strncmp(token_str, true_str, std::size(true_str) - 1) == 0)
		return ProjVar(true);

	if (strncmp(token_str, false_str, std::size(false_str) - 1) == 0)
		return ProjVar(false);

	return ProjVar::VarString(token_str, get_current().length);
}

ProjVar Parser::_parse_var_number() {
	// TODO: error detection

	float val = strtof(m_source + get_current().start, nullptr);

	return ProjVar(val);
}

ProjVar Parser::_parse_var_simple() {
	if (get_current().type == TokenType::String)
		return _parse_var_string(true);

	if (isdigit(m_source[get_current().start]) == 0)
		return _parse_var_number();

	return _parse_var_identifier();
}

ProjVar Parser::_parse_var_dict() {
	return ProjVar();
}

ProjVar Parser::_parse_var_array() {
	return ProjVar();
}

ProjVar Parser::_get_var() {
	_skip_useless();

	switch (get_current().type)
	{
	case TokenType::Identifier:
	case TokenType::String:
		return _parse_var_simple();
	case TokenType::Open_CurlyBracket:
		return _parse_var_dict();
	case TokenType::Open_SqBracket:
		return _parse_var_array();

	default:
		//! Report the error with more info
		throw std::runtime_error("error");
	}

	return ProjVar(0.0F);
}

std::pair<ProjVar::VarString, ProjVar> Parser::_get_kv() {
	_skip_useless();



	return std::pair<ProjVar::VarString, ProjVar>();
}

ProjVar::VarDict Parser::parse() {
	return ProjVar::VarDict();
}

#pragma endregion

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

	vector<Token> tokens{};

	tokenizer.put_all(tokens);

	Parser parser{source, length, tokens.data(), tokens.size()};

	return ProjVar(parser.parse());
}

