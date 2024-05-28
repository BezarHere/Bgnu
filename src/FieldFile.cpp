#include "FieldFile.hpp"
#include "Logger.hpp"
#include <fstream>
#include <string.h>

inline static bool is_identifier_char(char c) {
	return isalnum(c) || c == '_' || c == '.' || c == '-' || c == '+';
}

inline static bool is_number_identifier(const char *str, size_t len) {
	if (len == 0)
	{
		return false;
	}

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

	enum class SkipType : uint8_t
	{
		None,
		Useless,
		Empty
	};

	inline Parser(const Token *tks, const size_t count)
		: m_tokens{tks}, m_size{count} {
	}

	FieldVar::Dict parse();

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

	inline void _push_index() {
		m_indicies_stack.push_back(current_index);
	}

	inline void _pop_index() {
		current_index = m_indicies_stack.back();
		m_indicies_stack.pop_back();
	}

	FieldVar _parse_var();
	std::pair<FieldVar::String, FieldVar> _parse_var_kv();


	FieldVar::String _parse_var_string();
	FieldVar::Real _parse_var_number();
	FieldVar::Array _parse_var_array();
	FieldVar::Dict _parse_var_dict(bool body_dict = false);

	FieldVar _parse_var_identifier();
	FieldVar _parse_var_simple();

	NORETURN void _XUnexpectedToken(const string &msg = "") const {
		std::ostringstream ss{};

		ss << "Unexpected " << get_tk();

		if (!msg.empty())
		{
			ss << ", expected " << msg;
		}

		throw std::runtime_error(ss.str().c_str());
	}

private:
	const Token *m_tokens;
	const size_t m_size;
	vector<size_t> m_indicies_stack = {};
};

FieldVar::String Parser::_parse_var_string() {
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

FieldVar Parser::_parse_var_identifier() {
	const Token &token = get_tk();
	_advance_tk();

	const char *token_str = token.str;
	constexpr const char null_str[] = "null";
	constexpr const char true_str[] = "true";
	constexpr const char false_str[] = "false";

	if (strncmp(token_str, null_str, std::size(null_str) - 1) == 0)
	{
		return FieldVar(FieldVarType::Null);
	}

	if (strncmp(token_str, true_str, std::size(true_str) - 1) == 0)
	{
		return true;
	}

	if (strncmp(token_str, false_str, std::size(false_str) - 1) == 0)
	{
		return false;
	}

	return FieldVar::String(token_str, token.length);
}

FieldVar::Real Parser::_parse_var_number() {
	// TODO: error detection

	float val = strtof(get_tk().str, nullptr);

	// read this token, now we advance
	_advance_tk();

	return val;
}

FieldVar Parser::_parse_var_simple() {
	_skip_useless();

	if (get_tk().type == TKType::String)
		return _parse_var_string();

	if (is_number_identifier(get_tk().str, get_tk().length))
		return _parse_var_number();

	return _parse_var_identifier();
}

FieldVar::Dict Parser::_parse_var_dict(const bool body_dict) {
	_skip_useless();

	// skip starting '{' for non-body dicts
	if (!body_dict && get_tk().type == TKType::Open_CurlyBracket)
	{
		_advance_tk();
	}

	// std::cout << "dict started at " << get_tk() << '\n';

	bool expecting_separator = false;
	FieldVar::Dict dict{};

	_skip_empty();

	while (_can_read())
	{
		_push_index();
		_skip_empty();

		// checking for dict termination (either EOF for body dict or closing '}' for normal dicts)
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

		_pop_index();
		_skip_useless();

		if (expecting_separator)
		{
			// the dict has no key-value separator, but we expecting the next kv
			// if we didn't have a next kv, then it would have had stoped above
			if (get_tk().type == TKType::Comma || get_tk().type == TKType::Newline)
			{
				_advance_tk();
				expecting_separator = false;
				continue;
			}

			_XUnexpectedToken(format_join('\'', ValueSeparateOp, "' or '}'"));
		}

		// skip useless and new lines to the next value token
		_skip_empty();

		// parse next tokens as value key pairs to the constructed dict
		dict.emplace(_parse_var_kv());

		expecting_separator = true;
	}

	std::cout << "dict ended at " << get_tk() << '\n';

	return dict;
}

FieldVar::Array Parser::_parse_var_array() {
	_skip_useless();

	if (get_tk().type == TKType::Open_SqBracket)
	{
		_advance_tk();
	}

	bool expecting_separator = false;

	FieldVar::Array array{};

	_skip_empty();

	while (_can_read())
	{
		_skip_empty();

		// did we hit the closing bracket? skip it (to mark it read) and break
		if (get_tk().type == TKType::Close_SqBracket)
		{
			_advance_tk();
			break;
		}

		std::cout << "A1: " << get_tk() << '\n';

		if (expecting_separator)
		{
			if (get_tk().type == TKType::Comma)
			{
				_advance_tk();
				expecting_separator = false;
				continue;
			}

			_XUnexpectedToken(format_join('"', ValueSeparateOp, "\" or \"]\""));
		}


		// parse next tokens as value and added to the constructed array
		array.emplace_back(_parse_var());
		expecting_separator = true;
	}

	return array;
}

FieldVar Parser::_parse_var() {
	_skip_useless();

	std::cout << "parsing tk: " << get_tk() << '\n';

	switch (get_tk().type)
	{
	case TKType::Identifier:
	case TKType::String:
		return _parse_var_simple();
	case TKType::Open_CurlyBracket:
		return FieldVar(_parse_var_dict());
	case TKType::Open_SqBracket:
		return FieldVar(_parse_var_array());

	default:
		//! Report the error with more info
		throw std::runtime_error("error");
	}

	return FieldVar(0.0F);
}

std::pair<FieldVar::String, FieldVar> Parser::_parse_var_kv() {

	_skip_useless();

	FieldVar::String key = _parse_var_string();

	_skip_empty();

	if (get_tk().type != TKType::Column)
	{
		_XUnexpectedToken(format_join('"', ValueAssignOp, '"'));
	}

	_advance_tk();
	_skip_empty();

	return {key, _parse_var()};
}

FieldVar::Dict Parser::parse() {
	return _parse_var_dict(true);
}

#pragma endregion

#pragma region(writer)

class FieldFileWriter
{
public:
	inline FieldFileWriter(std::ostringstream &p_stream) : stream{p_stream} {}

	void start(const FieldVar::Dict &data);

	void write(FieldVar::Null data);
	void write(FieldVar::Bool data);
	void write(FieldVar::Real data);

	void write(const FieldVar::String &data);
	void write(const FieldVar::Array &data);
	void write(const FieldVar::Dict &data, bool striped = false);

	void write(const FieldVar &data);

	inline void write_indent() {
		stream << string(m_indent_level * 2, ' ');
	}

	std::ostringstream &stream;

private:
	uint32_t m_indent_level = 0;
};


void FieldFileWriter::start(const FieldVar::Dict &data) {
	m_indent_level = 0;
	write(data, true);
}

void FieldFileWriter::write(FieldVar::Null data) {
	stream << "null";
}

void FieldFileWriter::write(FieldVar::Bool data) {
	stream << (data ? "true" : "false");
}

void FieldFileWriter::write(FieldVar::Real data) {
	stream << data;
}

void FieldFileWriter::write(const FieldVar::String &data) {
	bool simple_string = data.length() < 32ULL;

	if (simple_string)
	{
		// check if the string is not simple (only identifier chars)
		for (const char c : data)
		{
			if (!is_identifier_char(c))
			{
				simple_string = false;
				break;
			}
		}
	}

	if (!simple_string)
	{
		stream << '"' << data << '"';
		return;
	}

	stream << data;
}

void FieldFileWriter::write(const FieldVar::Array &data) {
	stream << '[';
	for (const auto &var : data)
	{
		write(var);
		stream << ", ";
	}
	stream << ']';
}

void FieldFileWriter::write(const FieldVar::Dict &data, bool striped) {
	if (!striped)
	{
		stream << "{\n";
	}

	++m_indent_level;

	for (const auto &kv : data)
	{
		write_indent();
		stream << kv.first << ": ";
		write(kv.second);
		stream << '\n';
	}

	--m_indent_level;

	if (!striped)
	{
		write_indent();
		stream << '}';
	}
}

void FieldFileWriter::write(const FieldVar &data) {
	switch (data.get_type())
	{
	case FieldVarType::Null:
		write(nullptr);
		return;
	case FieldVarType::Boolean:
		write(data.get_bool());
		return;
	case FieldVarType::Real:
		write(data.get_real());
		return;
	case FieldVarType::String:
		write(data.get_string());
		return;
	case FieldVarType::Array:
		write(data.get_array());
		return;
	case FieldVarType::Dict:
		write(data.get_dict());
		return;
	default:
		stream << "UNKNOWN";
		return;
	}
}

#pragma endregion

FieldVar FieldFile::load(const string &filepath) {
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

FieldVar FieldFile::read(const char *source, size_t length) {
	Tokenizer tokenizer{source, length};

	vector<Token> tokens{};

	tokenizer.put_all(tokens);

	// for (const auto &tk : tokens)
	// {
	// 	std::cout << tk << '\n';
	// }

	Parser parser{tokens.data(), tokens.size()};

	return FieldVar(parser.parse());
}

void FieldFile::dump(const string &filepath, const FieldVar::Dict &data) {
	string source = write(data);

	std::ofstream out{filepath};

	out << source;
}

string FieldFile::write(const FieldVar::Dict &data) {
	std::ostringstream output{};
	FieldFileWriter writer{output};
	writer.write(data);
	return output.str();
}

