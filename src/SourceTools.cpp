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
		Type type;
		IndexRange name;
		IndexRange value; // or anything right of the statement
		uint32_t line;
	};

	static Token get_next_tk(const StrBlob &source);
};

vector<string> SourceTools::get_dependencies(const StrBlob &file) {
	return vector<string>();
}

CPreprocessor::Token CPreprocessor::get_next_tk(const StrBlob &source) {


	return Token();
}
