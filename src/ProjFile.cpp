#include "ProjFile.hpp"
#include "Console.hpp"
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
	static constexpr char ValueAssignOp = ':';
	static constexpr char ValueSeparateOp = ',';

	inline Parser(const Token *tks, const size_t count)
		: m_tokens{tks}, m_size{count} {
	}

	ProjVar::VarDict parse();

	static bool is_useless_token(const Token &token) {
		if (token.length == 0)
		{
			return true;
		}

		return token.type == TokenType::Whitespace || token.type == TokenType::Unknown;
	}

	static bool is_empty_token(const Token &token) {
		return is_useless_token(token) || token.type == TokenType::Newline;
	}

	// current ptr
	inline const Token *current() const noexcept { return &m_tokens[current_index]; }
	inline const Token &get_tk() const noexcept { return m_tokens[current_index]; }

	inline size_t size() const noexcept { return m_size; }
	inline const Token *data() const noexcept { return m_tokens; }

	size_t current_index;

private:
	ALWAYS_INLINE void _advance_tk() {
		++current_index;
	}

	ALWAYS_INLINE void _advance_tk(size_t amount) {
		if (amount == 0)
		{
			//! ERROR?
		}

		current_index += amount;
	}

	ALWAYS_INLINE bool _can_read() const {
		return current_index < m_size - 1;
	}

	inline void _skip_useless() {
		while (current_index < m_size && Parser::is_useless_token(m_tokens[current_index]))
		{
			++current_index;
		}
	}

	inline void _skip_empty() {
		while (current_index < m_size && Parser::is_empty_token(m_tokens[current_index]))
		{
			++current_index;
		}
	}

	ProjVar _parse_var();
	std::pair<ProjVar::VarString, ProjVar> _parse_var_kv();


	ProjVar::VarString _parse_var_string() const;
	ProjVar::VarNumber _parse_var_number() const;
	ProjVar::VarArray _parse_var_array();
	ProjVar::VarDict _parse_var_dict(const bool body_dict = false);

	ProjVar _parse_var_identifier() const;
	ProjVar _parse_var_simple();

	NORETURN void _XUnexpectedToken(const string &expected = "") const {
		char buffer[inner::ExceptionBufferSz]{};

		if (!expected.empty())
		{
			sprintf_s(
				buffer, "Unexpected token at %u:%u, expected '%s'",
				get_tk().pos.line, get_tk().pos.column, expected.c_str()
			);
		}
		else
		{
			sprintf_s(buffer, "Unexpected token at %u:%u", get_tk().pos.line, get_tk().pos.column);
		}

		throw std::runtime_error(buffer);
	}

private:
	const Token *m_tokens;
	const size_t m_size;
};

ProjVar::VarString Parser::_parse_var_string() const {
	if (get_tk().type != TokenType::String)
	{
		return {get_tk().str, get_tk().length};
	}

	if (get_tk().length < 2)
	{
		return {};
	}

	return {get_tk().str + 1, get_tk().length - 2};
}

ProjVar Parser::_parse_var_identifier() const {
	const char *token_str = get_tk().str;
	constexpr const char null_str[] = "null";
	constexpr const char true_str[] = "true";
	constexpr const char false_str[] = "false";

	if (strncmp(token_str, null_str, std::size(null_str) - 1) == 0)
	{
		return ProjVar(ProjVarType::Null);
	}

	if (strncmp(token_str, true_str, std::size(true_str) - 1) == 0)
	{
		return true;
	}

	if (strncmp(token_str, false_str, std::size(false_str) - 1) == 0)
	{
		return false;
	}

	return ProjVar::VarString(token_str, get_tk().length);
}

ProjVar::VarNumber Parser::_parse_var_number() const {
	// TODO: error detection

	float val = strtof(get_tk().str, nullptr);

	return val;
}

ProjVar Parser::_parse_var_simple() {
	_skip_useless();

	if (get_tk().type == TokenType::String)
		return _parse_var_string();

	if (is_number_identifier(get_tk().str, get_tk().length))
		return _parse_var_number();

	return _parse_var_identifier();
}

ProjVar::VarDict Parser::_parse_var_dict(const bool body_dict) {
	_skip_useless();

	// skip starting '{' for non-body dicts
	if (!body_dict && get_tk().type == TokenType::Open_CurlyBracket)
	{
		_advance_tk();
	}

	ProjVar::VarDict dict{};

	while (_can_read())
	{
		_skip_empty();

		// parse next tokens as value and added to the constructed array
		dict.emplace(_parse_var_kv());

		_skip_empty();
		const bool has_leading_comma = get_tk().type == TokenType::Comma;

		// did we hit a comma after value? then skip it
		if (has_leading_comma)
		{
			_advance_tk();
			_skip_empty();
		}

		if (body_dict)
		{
			// for body dicts, we close on EOF
			if (get_tk().type == TokenType::Eof)
			{
				break;
			}

		}
		else
		{

			// did we hit the closing bracket? skip it (to mark it read) and break
			// if we didn't hit a closing bracket, then there *must be a next value*
			if (get_tk().type == TokenType::Close_CurlyBracket)
			{
				_advance_tk();
				break;
			}
		}

		// the list has a next value with no comma separating them
		// if it didn't have a next value, then it would have had stoped above
		if (!has_leading_comma)
		{
			_XUnexpectedToken(format_join('\'', ValueSeparateOp, "' or '}'"));
		}

	}

	return dict;
}

ProjVar::VarArray Parser::_parse_var_array() {
	_skip_useless();

	if (get_tk().type == TokenType::Open_SqBracket)
	{
		_advance_tk();
	}

	ProjVar::VarArray array{};
	while (_can_read())
	{
		_skip_empty();

		// parse next tokens as value and added to the constructed array
		array.emplace_back(_parse_var());

		_skip_empty();
		const bool has_leading_comma = get_tk().type == TokenType::Comma;

		// did we hit a comma after value? then skip it
		if (has_leading_comma)
		{
			_advance_tk();
			_skip_empty();
		}

		// did we hit the closing bracket? skip it (to mark it read) and break
		// if we didn't hit a closing bracket, then there must be a next value
		if (get_tk().type == TokenType::Close_SqBracket)
		{
			_advance_tk();
			break;
		}

		// the list has a next value with no comma separating them
		// if it didn't have a next value, then it would have had stoped above
		if (!has_leading_comma)
		{
			_XUnexpectedToken(format_join('\'', ValueSeparateOp, "' or ']'"));
		}

	}

	return array;
}

ProjVar Parser::_parse_var() {
	_skip_useless();

	switch (get_tk().type)
	{
	case TokenType::Identifier:
	case TokenType::String:
		return _parse_var_simple();
	case TokenType::Open_CurlyBracket:
		return ProjVar(_parse_var_dict());
	case TokenType::Open_SqBracket:
		return ProjVar(_parse_var_array());

	default:
		//! Report the error with more info
		throw std::runtime_error("error");
	}

	return ProjVar(0.0F);
}

std::pair<ProjVar::VarString, ProjVar> Parser::_parse_var_kv() {

	_skip_useless();
	ProjVar::VarString key = _parse_var_string();

	_skip_empty();

	if (get_tk().type != TokenType::Column)
	{
		_XUnexpectedToken(string(1, ValueAssignOp));
	}

	_advance_tk();
	_skip_empty();

	return {key, _parse_var()};
}

ProjVar::VarDict Parser::parse() {
	return _parse_var_dict(true);
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

	Parser parser{tokens.data(), tokens.size()};

	return ProjVar(parser.parse());
}

