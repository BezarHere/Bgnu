#include "FieldFile.hpp"
#include "FileTools.hpp"
#include "misc/StringCursor.hpp"
#include "Logger.hpp"
#include <fstream>
#include <string.h>

constexpr uint32_t FieldFileVersion = 0x01'01;

constexpr int base_2 = 2;
constexpr int base_8 = 8;
constexpr int base_10 = 10;
constexpr int base_16 = 16;

inline static bool IsIdentifierChar(char chr) {
	return isalnum(chr) || chr == '_' || chr == '.' || chr == '-' || chr == '+';
}

inline static bool IsIdentifierCharExtended(char chr) {
	return IsIdentifierChar(chr) || chr == ':' || chr == '.';
}

inline static bool IsIdentifierFirstChar(char chr) {
	return IsIdentifierCharExtended(chr) && !isdigit(chr);
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

enum ScanNumberType : uint8_t
{
	eScanNType_None = 0,
	eScanNType_Float = 1,
	eScanNType_Int = 2
};

static inline ScanNumberType ScanForNumberType(StrBlob str);
static inline bool IsValidIdentifierName(StrBlob str);
static inline FieldVar::Int ParseInt(const string_char *str, size_t max_length);
static inline int ScanForIntBase(const string_char *str, size_t max_length);
static inline unsigned IntBaseStartOffset(int base);

#pragma region(tokenizer)

enum class TKType : uint8_t
{
	Unknown,
	Invalid,
	Comment,
	Eof,

	Whitespace,
	Newline,

	Assign,
	Comma,

	Open_SqBracket,
	Close_SqBracket,
	Open_CurlyBracket,
	Close_CurlyBracket,

	// can be a number, a boolean, null or anything
	Identifier,
	String,
};

enum StringParseFlags : uint8_t
{
	eStrPFlag_None = 0x00,
	eStrPFlag_Multiline = 0x01,
	eStrPFlag_Literal = 0x02,
};

struct SourcePosition
{
	SourcePosition() = default;
	SourcePosition(uint32_t p_line, uint32_t p_column) : line{p_line}, column{p_column} {}

	uint32_t line = 0, column = 0;
};

struct Token
{
	TKType type;

	string_cursor str;
	size_t length;

	SourcePosition pos;
};

static inline constexpr bool IsAssignTokenType(TKType type);

namespace std
{
	ostream &operator<<(ostream &stream, const SourcePosition &pos) {
		return stream << pos.line << ':' << pos.column;
	}

	ostream &operator<<(ostream &stream, const Token &tk) {
		return stream
			<< "TK('"
			<< string(tk.str.c_str(), tk.length)
			<< "' ["
			<< tk.str.position()
			<< ':'
			<< tk.length
			<< "], "
			<< tk.pos
			<< ", "
			<< (int)tk.type
			<< ')';
	}
}

class Tokenizer
{
public:
	static constexpr char CommentChar = '#';

	inline Tokenizer(const char *p_source, size_t p_length) : m_source{p_source}, m_length{p_length} {}

	inline Token get_next();

	inline void put_all(vector<Token> &output) {
		const size_t old_index = index;
		// std::cout << "tokens output start: " << output.size() << '\n';

		while (index < m_length)
		{
			output.push_back(get_next());
			if (output.back().type == TKType::Eof)
			{
				break;
			}

			// std::cout << "token: " << output.back() << ", str pos: " << index << ' ' << output.size() << '\n';
		}

		// std::cout << "tokens count: " << output.size() << " str pos: " << index << '\n';
		index = old_index;
	}

	inline SourcePosition get_pos() const noexcept { return {line + 1U, uint32_t(index - line_start) + 1U}; }

	size_t index = 0;
	uint32_t line = 0;
	size_t line_start = 0;


private:
	inline string_cursor _get_current_str() const { return {m_source, index}; }

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

	// found end of file
	if (token.type == TKType::Eof)
	{
		return token;
	}

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
			{
				continue;
			}

			Logger::error("_tk_string: Unclosed string at %llu:%llu", line, i - line_start);
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
	if (!IsIdentifierChar(m_source[index]))
		return {TKType::Unknown, _get_current_str(), 1, get_pos()};

	size_t result_end = m_length;

	for (size_t i = index; i < m_length; ++i)
	{
		if (IsIdentifierCharExtended(m_source[i]))
		{
			continue;
		}

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
		MATCH_SPAN_CHAR('=', TKType::Assign);
		MATCH_SPAN_CHAR(',', TKType::Comma);
#undef MATCH_SPAN_CHAR

	default:
		return _tk_identifier();
	}

	return Token();
}


inline ScanNumberType ScanForNumberType(StrBlob str) {
	if (str.empty())
	{
		return eScanNType_None;
	}

	if (str[0] == '-' || str[0] == '+')
	{
		str = str.slice(1);
	}

	const size_t dot_pos = string_tools::find(str.data, str.size, [](char c) {return c == '.';});

	if (dot_pos != npos)
	{
		if (ScanForNumberType(str.slice(0, dot_pos)) == eScanNType_None)
		{
			return eScanNType_None;
		}
		// check before the dot and after it
		return ScanForNumberType(str.slice(dot_pos + 1)) == eScanNType_Int ? eScanNType_Float : eScanNType_None;
	}

	return string_tools::all_of(str.data, str.size, isdigit) ? eScanNType_Int : eScanNType_None;
}

inline bool IsValidIdentifierName(StrBlob str) {
	if (str.empty())
	{
		return false;
	}

	if (!IsIdentifierFirstChar(str[0]))
	{
		return false;
	}

	return string_tools::all_of(str.data, str.size, IsIdentifierCharExtended);
}

inline FieldVar::Int ParseInt(const string_char *str, size_t max_length) {
	const int base = ScanForIntBase(str, max_length);

	const unsigned offset = IntBaseStartOffset(base);

	// bug! how can that even happen? ScanForIntBase will ret 10 for invalid number (base 10 has 0 off)
	if (max_length <= offset)
	{
		return 0;
	}

	str += offset;
	max_length -= offset;

	return strtoll(str, nullptr, base);
}

inline int ScanForIntBase(const string_char *str, size_t max_length) {
	// needs at least two chars to define a base '0x', '0b'
	if (max_length < 2)
	{
		return base_10;
	}

	if (str[0] != '0')
	{
		return base_10;
	}

	if (str[1] == 'x' || str[1] == 'X')
	{
		return base_16;
	}

	if (str[1] == 'b' || str[1] == 'B')
	{
		return base_2;
	}

	if (isdigit(str[1]))
	{
		return base_8;
	}

	return base_10;
}

inline unsigned IntBaseStartOffset(const int base) {
	if (base == base_16 || base == base_2)
	{
		return 2U;
	}

	if (base == base_8)
	{
		return 1U;
	}

	return 0U;
}

inline constexpr bool IsAssignTokenType(TKType type) {
	return type == TKType::Assign;
}

#pragma endregion

#pragma region(parser)

class Parser
{
public:
	static constexpr char ValueAssignOp = '=';
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

		// return token.type == TKType::Whitespace || token.type == TKType::Unknown;
		return token.type == TKType::Whitespace;
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
			amount++;
		}

		current_index += amount;
	}

	ALWAYS_INLINE bool _can_read() const {
		return current_index < m_size - 1 && m_size > 0;
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
	FieldVar::Real _parse_var_float();
	FieldVar::Int _parse_var_int();
	FieldVar::Array _parse_var_array();
	FieldVar::Dict _parse_var_dict(bool body_dict = false);

	FieldVar _parse_var_identifier();
	FieldVar _parse_var_simple();

	NORETURN void _XUnexpectedToken(const string &msg = "") const {
		std::ostringstream output{};

		output << "Unexpected " << get_tk();

		if (!msg.empty())
		{
			output << ", expected " << msg;
		}

		throw std::runtime_error(output.str().c_str());
	}

private:
	const Token *m_tokens;
	const size_t m_size;
	vector<size_t> m_indicies_stack;
};

FieldVar::String Parser::_parse_var_string() {
	const Token &token = get_tk();
	_advance_tk();

	if (token.type != TKType::String)
	{
		return {token.str.c_str(), token.length};
	}

	if (token.length < 2)
	{
		return {};
	}

	return {token.str.c_str() + 1, token.length - 2};
}

FieldVar Parser::_parse_var_identifier() {
	const Token &token = get_tk();
	_advance_tk();

	const char *token_str = token.str.c_str();
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

FieldVar::Real Parser::_parse_var_float() {
	// TODO: error detection

	float val = strtof(get_tk().str.c_str(), nullptr);

	// read this token, now we advance
	_advance_tk();

	return val;
}

FieldVar::Int Parser::_parse_var_int() {
	// TODO: error detection

	FieldVar::Int val = ParseInt(get_tk().str.c_str(), get_tk().length);

	// had read this token, now we advance
	_advance_tk();

	return val;
}

FieldVar Parser::_parse_var_simple() {
	_skip_useless();

	if (get_tk().type == TKType::String)
	{
		return _parse_var_string();
	}

	const ScanNumberType n_type = ScanForNumberType({get_tk().str.c_str(), get_tk().length});
	if (n_type == eScanNType_Float)
	{
		return _parse_var_float();
	}
	
  if (n_type == eScanNType_Int)
	{
		return _parse_var_int();
	}

	return _parse_var_identifier();
}

FieldVar::Dict Parser::_parse_var_dict(const bool body_dict) {
	_skip_useless();

	// skip starting '{' for non-body dicts
	if (!body_dict && get_tk().type == TKType::Open_CurlyBracket)
	{
		_advance_tk();
	}

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

		if (!_can_read())
		{
			break;
		}
		// std::cout << "0[0]: " << current_index << ' ' << get_tk() << '\n';

		if (expecting_separator)
		{
			// std::cout << "0[1]: " << current_index << ' ' << get_tk() << '\n';
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
		// std::cout << "1: " << current_index << ' ' << *current() << '\n';
		dict.emplace(_parse_var_kv());
		// std::cout << "2: " << current_index << ' ' << *current() << '\n';

		expecting_separator = true;
	}

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

	
	// std::cout << "4: " << current_index << ' ' << *current() << '\n';

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

	if (!_can_read())
	{
		//! ERROR?
		return {};
	}


	FieldVar::String key = _parse_var_string();

	_skip_empty();

	if (!_can_read())
	{
		//! ERROR?
		return {key, {}};
	}

	if (!IsAssignTokenType(get_tk().type))
	{
    // helpful note?
    if (get_tk().str[0] == ':')
    {
      Logger::note("KeyValue pairs in FieldVar data files expect '=' unlike json-ish data files ':'");
    }
    
		_XUnexpectedToken(format_join('"', ValueAssignOp, '"'));
	}

	// std::cout << "3: " << current_index << ' ' << *current() << '\n';
	_advance_tk();
	// std::cout << "3.5: " << current_index << ' ' << *current() << *(current() + 1)  << *(current() + 2) << '\n';
	_skip_empty();

	if (!_can_read())
	{
		//! ERROR?
		return {key, {}};
	}


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
	void write(FieldVar::Int data);
	void write(FieldVar::Real data);

	void write(const FieldVar::String &data);
	void write(const FieldVar::Array &data);
	void write(const FieldVar::Dict &data, bool striped = false);

	void write(const FieldVar &data);

	inline void write_indent() {
		stream << string(m_indent_level * 2UL, ' ');
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
  (void)data;
	stream << "null";
}

void FieldFileWriter::write(FieldVar::Bool data) {
	stream << (data ? "true" : "false");
}

void FieldFileWriter::write(FieldVar::Int data) {
	stream << data;
}

void FieldFileWriter::write(FieldVar::Real data) {
	stream << data;
}

void FieldFileWriter::write(const FieldVar::String &data) {
	const bool long_string = data.length() >= 45ULL;
	const bool ambiguous_to_number = ScanForNumberType({data.c_str(), data.length()});

	const bool replicable_by_identifier = !long_string && !ambiguous_to_number;

	if (replicable_by_identifier && IsValidIdentifierName({data.c_str(), data.length()}))
	{
		stream << data;
		return;
	}

	stream << '"' << data << '"';
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
		++m_indent_level;
	}


	for (const auto &kv_pair : data)
	{
		write_indent();
		write(kv_pair.first);
		stream << ' ' << Parser::ValueAssignOp << ' ';
		write(kv_pair.second);
		stream << '\n';
	}


	if (!striped)
	{
		--m_indent_level;
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
	case FieldVarType::Integer:
		write(data.get_int());
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

FieldVar FieldFile::load(const FilePath &filepath) {
	const string text = FileTools::read_str(filepath);
	// std::cout << "\n\n[[[\n" << text << "\n]]]" << text.size() << "\n\n";

	return read(text.c_str(), text.length());
}

FieldVar FieldFile::read(const string_char *source, size_t length) {
	Tokenizer tokenizer{source, length};

	vector<Token> tokens{};

	tokenizer.put_all(tokens);

	Parser parser{tokens.data(), tokens.size()};

	return FieldVar(parser.parse());
}

void FieldFile::dump(const FilePath &filepath, const FieldVar::Dict &data) {
	string source = write(data);

	std::ofstream out{filepath};

	out << source;
}

string FieldFile::write(const FieldVar::Dict &data) {
	std::ostringstream output{};
	FieldFileWriter writer{output};
	writer.write(data, true);
	return output.str();
}
