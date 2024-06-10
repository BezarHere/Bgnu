#pragma once
#include "../base.hpp"
#include "../Range.hpp"
#include "../StringTools.hpp"

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
		Token(const string_char *source, const IndexRange &name, const IndexRange &value);

		inline bool is_valid() const {
			return (int)type >= (int)Type::Include;
		}

		Type type = Type::None;
		IndexRange name;
		IndexRange value; // or anything right of the statement
		uint32_t line = 0;
	};

	static constexpr pair<const char *, Type> TypeNamePairs[] = {
		{"include", Type::Include},

		{"define", Type::Define},
		{"undef", Type::Undefine},

		{"if", Type::If},
		{"else", Type::Else},
		{"elif", Type::Elif},
		{"endif", Type::EndIf},

		{"ifdef", Type::IfDefined},
		{"ifndef", Type::IfNotDefined},

		{"error", Type::Error},
		{"warning", Type::Warning},

		{"pragma", Type::Pragma},
	};


	static void gather_all_tks(const StrBlob &source, vector<Token> &output);
	static Token get_next_tk(const StrBlob &source);

	static string get_include_path(const StrBlob &value);

	static constexpr Type _get_tk_type(const StrBlob &name);

	static Token _read_tk_inline(const StrBlob &source);
	static Token _parse_tk(const StrBlob &line);

	static size_t _escape(const StrBlob &source, const MutableStrBlob &destination);

};
