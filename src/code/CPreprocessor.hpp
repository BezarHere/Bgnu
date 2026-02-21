#pragma once
#include <string_view>

#include "../Range.hpp"
#include "../StringTools.hpp"
#include "../base.hpp"
#include "../io/CharSource.hpp"

struct CPreprocessor
{

  enum class Type : uint8_t {
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
    Token(const string &name, const string &value);

    inline bool is_valid() const { return (int)type >= (int)Type::Include; }

    Type type = Type::None;
    string value;  // or anything right of the statement
    uint32_t line = 0;
  };

  static constexpr pair<const char *, Type> TypeNamePairs[] = {
    { "include", Type::Include },

    { "define", Type::Define },   { "undef", Type::Undefine },

    { "if", Type::If },           { "else", Type::Else },
    { "elif", Type::Elif },       { "endif", Type::EndIf },

    { "ifdef", Type::IfDefined }, { "ifndef", Type::IfNotDefined },

    { "error", Type::Error },     { "warning", Type::Warning },

    { "pragma", Type::Pragma },
  };

  using source_t = CharSource &;

  static void gather_all_tks(const StrBlob &input, vector<Token> &output);
  static Token get_next_tk(source_t source);

  static string get_include_path(const std::string_view &value);

private:
  static constexpr Type _get_tk_type(const StrBlob &name);

  static Token _parse_preprocessor_line(source_t source);
  static string _read_preprocessor_name(source_t source);
  static string _read_preprocessor_value(source_t source);

  static string _read_value(source_t source);

  static void _try_skip_comment(source_t source);
  static void _try_skip_to_usable(source_t source);

  static string _escape(const StrBlob &source);
};
