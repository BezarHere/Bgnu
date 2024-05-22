#include "ProjFile.hpp"
#include "Console.hpp"
#include <fstream>
#include <string.h>

inline static bool is_identifier_char(char c) {
	return isalnum(c) || c == '_' || c == '.' || c == '-' || c == '+';
}

inline static bool is_number_identifier(const char *str, size_t len) {
	if (len == 0)
		return false;

	// skip the minus or plus signs, if there are any
	if (*str == '-' || *str == '+')
	{
		++str;
		--len;
	}

	return isdigit(*str);
}

#pragma region(tokenizer)
enum class TKType : uint8_t
{
	Unknown,
	Invalid,
	Comment,
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
	TKType type;

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

class Tokenizer
{
public:
	static constexpr char CommentChar = '#';

	inline Tokenizer(const char *p_source, size_t p_length) : m_source{p_source}, m_length{p_length} {}

	inline Token get_next();

	inline void put_all(vector<Token> &output) {
		const size_t old_index = index;

		while (index < m_length)
		{
			output.emplace_back(get_next());
			if (output.back().type == TKType::Eof)
				break;
		}

		index = old_index;
	}

	inline TKPos get_pos() const noexcept { return {line, uint16_t(index - line_start)}; }

	size_t index = 0;
	uint16_t line = 0;
	size_t line_start = 0;


private:
	inline const char *_get_current_str() const { return m_source + index; }

	template <typename Pred>
	inline size_t _get_length(Pred &&pred) const {
		for (size_t i = index; i < m_length; ++i)
		{
			if (!pred(m_source[i]))
				return i - index;
		}
		return m_length - index;
	}

	template <typename Pred>
	inline Token _tk_span(TKType type, Pred &&pred) const {
		const size_t length = _get_length(pred);
		return {type, _get_current_str(), length, get_pos()};
	}

	Token _read_comment() const;

	Token _tk_string(StringParseFlags flags) const;

	Token _tk_identifier() const;

	inline Token _read_tk() const;

private:
	const char *const m_source;
	const size_t m_length;
};

inline Token Tokenizer::get_next() {
	Token token = _read_tk();

	if (token.length == 0)
	{
		//? should be an error here?
		++index;
	}

	// advance by the tokens length
	index += token.length;

	// skip comments, return the next thing
	if (token.type == TKType::Comment)
	{
		return get_next();
	}

	if (token.type == TKType::Newline)
	{
		line += token.length;
		line_start = index;
	}

	return token;
}

Token Tokenizer::_read_comment() const {
	size_t end = m_length;
	for (size_t i = index; i < m_length; i++)
	{
		if (m_source[i] == '\n')
		{
			end = i;
			break;
		}
	}

	return {TKType::Comment, _get_current_str(), end - index, get_pos()};
}

Token Tokenizer::_tk_string(const StringParseFlags flags) const {
	const char start = m_source[index];

	size_t end_index = 0;

	for (size_t i = index + 1; i < m_length; ++i)
	{
		if (!HAS_FLAG(flags, eStrPFlag_Literal) && m_source[i] == '\\')
		{
			++i;
			continue;
		}

		if (m_source[i] == '\n' || m_source[i] == '\r')
		{
			if (HAS_FLAG(flags, eStrPFlag_Multiline))
				continue;

			std::cerr << "Unclosed string at " << line << ':' << (i - line_start) << '\n';
			end_index = i;
			break;
		}

		if (m_source[i] == start)
		{
			end_index = i;
			break;
		}

	}

	return {TKType::String, _get_current_str(), (end_index + 1) - index, get_pos()};
}

Token Tokenizer::_tk_identifier() const {
	if (!is_identifier_char(m_source[index]))
		return {TKType::Unknown, _get_current_str(), 1, get_pos()};

	size_t result_end = m_length;

	for (size_t i = index; i < m_length; ++i)
	{
		if (is_identifier_char(m_source[i]))
			continue;

		result_end = i;
		break;
	}

	return {TKType::Identifier, _get_current_str(), result_end - index, get_pos()};
}

inline Token Tokenizer::_read_tk() const {
	if (index >= m_length)
	{
		return {TKType::Eof, _get_current_str(), 1, get_pos()};
	}

	switch (m_source[index])
	{
	case '\0':
		return {TKType::Eof, _get_current_str(), 1, get_pos()};
	case '\n':
	case '\r':
		return _tk_span(TKType::Newline, [](char c) { return c == '\n' || c == '\r'; });
	case '\t':
	case ' ':
		return _tk_span(TKType::Whitespace, [](char c) { return c == ' ' || c == '\t'; });

	case CommentChar:
		return _read_comment();

	case '"':
		return _tk_string(eStrPFlag_None);

#define MATCH_SPAN_CHAR(match_char, type) case match_char: return _tk_span(type, [](char val) { return val == (match_char); } )
		MATCH_SPAN_CHAR('{', TKType::Open_CurlyBracket);
		MATCH_SPAN_CHAR('}', TKType::Close_CurlyBracket);
		MATCH_SPAN_CHAR('[', TKType::Open_SqBracket);
		MATCH_SPAN_CHAR(']', TKType::Close_SqBracket);
		MATCH_SPAN_CHAR(':', TKType::Column);
		MATCH_SPAN_CHAR(',', TKType::Comma);
#undef MATCH_SPAN_CHAR

	default:
		return _tk_identifier();
	}

	return Token();
}

namespace std
{
	ostream &operator<<(ostream &stream, const TKPos &pos) {
		return stream << pos.line << ':' << pos.column;
	}

	ostream &operator<<(ostream &stream, const Token &tk) {
		return stream << "TK('" << string(tk.str, tk.length) << "' [" << tk.length << "], " << tk.pos << ", " << (int)tk.type << ')';
	}
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

		return token.type == TKType::Whitespace || token.type == TKType::Unknown;
	}

	static bool is_empty_token(const Token &token) {
		return is_useless_token(token) || token.type == TKType::Newline;
	}

	// current ptr
	inline const Token *current() const noexcept { return &m_tokens[current_index]; }
	inline const Token &get_tk() const noexcept { return m_tokens[current_index]; }

	inline size_t size() const noexcept { return m_size; }
	inline const Token *data() const noexcept { return m_tokens; }

	size_t current_index = 0;

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


	ProjVar::VarString _parse_var_string();
	ProjVar::VarNumber _parse_var_number();
	ProjVar::VarArray _parse_var_array();
	ProjVar::VarDict _parse_var_dict(const bool body_dict = false);

	ProjVar _parse_var_identifier();
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

ProjVar::VarString Parser::_parse_var_string() {
	const Token &tk = get_tk();
	_advance_tk();

	if (tk.type != TKType::String)
	{
		return {tk.str, tk.length};
	}

	if (tk.length < 2)
	{
		return {};
	}

	return {tk.str + 1, tk.length - 2};
}

ProjVar Parser::_parse_var_identifier() {
	const Token &token = get_tk();
	_advance_tk();

	const char *token_str = token.str;
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

	return ProjVar::VarString(token_str, token.length);
}

ProjVar::VarNumber Parser::_parse_var_number() {
	// TODO: error detection

	float val = strtof(get_tk().str, nullptr);

	// read this token, now we advance
	_advance_tk();

	return val;
}

ProjVar Parser::_parse_var_simple() {
	_skip_useless();

	if (get_tk().type == TKType::String)
		return _parse_var_string();

	if (is_number_identifier(get_tk().str, get_tk().length))
		return _parse_var_number();

	return _parse_var_identifier();
}

ProjVar::VarDict Parser::_parse_var_dict(const bool body_dict) {
	_skip_useless();

	// skip starting '{' for non-body dicts
	if (!body_dict && get_tk().type == TKType::Open_CurlyBracket)
	{
		_advance_tk();
	}

	ProjVar::VarDict dict{};

	std::cout << "can read? " << _can_read() << ' ' << current_index << ' ' << m_size << '\n';

	while (_can_read())
	{
		_skip_empty();

		std::cout << "1: " << get_tk() << '\n';

		// parse next tokens as value key pairs to the constructed dict
		dict.emplace(_parse_var_kv());


		std::cout << "2: " << get_tk() << '\n';

		_skip_useless();
		const bool has_kv_separator = get_tk().type == TKType::Comma || get_tk().type == TKType::Newline;

		// did we hit a comma after value? then skip it
		if (has_kv_separator)
		{
			_advance_tk();
			_skip_empty();
		}

		std::cout << "3: " << get_tk() << '\n';

		if (body_dict)
		{
			// for body dicts, we close on EOF
			if (get_tk().type == TKType::Eof)
			{
				break;
			}

		}
		else
		{

			// did we hit the closing bracket? skip it (to mark it read) and break
			// if we didn't hit a closing bracket, then there *must be a next value*
			if (get_tk().type == TKType::Close_CurlyBracket)
			{
				_advance_tk();
				break;
			}
		}

		// the dict has no key-value separator, but we expecting the next kv
		// if we didn't have a next kv, then it would have had stoped above
		if (!has_kv_separator)
		{
			_XUnexpectedToken(format_join('\'', ValueSeparateOp, "' or '}'"));
		}

	}

	return dict;
}

ProjVar::VarArray Parser::_parse_var_array() {
	_skip_useless();

	if (get_tk().type == TKType::Open_SqBracket)
	{
		_advance_tk();
	}

	ProjVar::VarArray array{};
	while (_can_read())
	{
		_skip_empty();

		std::cout << "A1: " << get_tk() << '\n';

		// parse next tokens as value and added to the constructed array
		array.emplace_back(_parse_var());


		std::cout << "A2: " << get_tk() << '\n';
		_skip_empty();
		const bool has_leading_comma = get_tk().type == TKType::Comma;

		// did we hit a comma after value? then skip it
		if (has_leading_comma)
		{
			_advance_tk();
			_skip_empty();
		}


		std::cout << "A3: " << get_tk() << '\n';

		// did we hit the closing bracket? skip it (to mark it read) and break
		// if we didn't hit a closing bracket, then there must be a next value
		if (get_tk().type == TKType::Close_SqBracket)
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
	case TKType::Identifier:
	case TKType::String:
		return _parse_var_simple();
	case TKType::Open_CurlyBracket:
		return ProjVar(_parse_var_dict());
	case TKType::Open_SqBracket:
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

	if (get_tk().type != TKType::Column)
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
	text.resize(size);

	return read(text.c_str(), text.length());
}

ProjVar ProjFile::read(const char *source, size_t length) {
	Tokenizer tokenizer{source, length};

	vector<Token> tokens{};

	tokenizer.put_all(tokens);

	// for (const auto &tk : tokens)
	// {
	// 	std::cout << tk << '\n';
	// }

	Parser parser{tokens.data(), tokens.size()};

	return ProjVar(parser.parse());
}

