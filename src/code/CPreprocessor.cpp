#include "CPreprocessor.hpp"

#include "CodeTokenizer.hpp"

constexpr auto whitespace_no_newlines = [](char cur) {
  return string_tools::is_whitespace(cur) && !string_tools::is_newline(cur);
};

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

void CPreprocessor::gather_all_tks(const StrBlob &input, vector<Token> &output) {
  string processed = _escape(input);

  CharSource source = { processed };

  while (!source.depleted())
  {
    Token token = get_next_tk(source);

    if (token.type != Type::None)
    {
      output.push_back(token);
    }

    if (source.depleted())
    {
      break;
    }

    LOG_ASSERT(source.current() == '\n');
    source.skip(1);
  }
}

CPreprocessor::Token CPreprocessor::get_next_tk(source_t source) {

  bool non_preprocessor_line = false;
  while (!source.depleted())
  {
    _try_skip_comment(source);

    const char cur_chr = source.get();

    if (non_preprocessor_line)
    {
      if (cur_chr == '\n')
      {
        non_preprocessor_line = false;
      }

      continue;
    }

    if (string_tools::is_whitespace(cur_chr))
    {
      continue;
    }

    if (cur_chr == '#')
    {
      // go back a character, so we are pointing at the '#'
      source.seek(-1, SeekOrigin::Current);
      return _parse_preprocessor_line(source);
    }

    non_preprocessor_line = true;
  }

  return Token();
}

string CPreprocessor::get_include_path(const std::string_view &value) {
  constexpr auto is_opening = [](string_char value) { return value == '"' || value == '<'; };
  constexpr auto is_closing = [](string_char value) { return value == '"' || value == '>'; };

  // pos of the opening quote/bracket
  size_t opening = CodeTokenizer::match_unescaped({ value.data(), value.length() }, is_opening);

  if (opening == npos)
  {
    return "";
  }

  opening = opening + 1;

  // pos of the closing quote/bracket
  const size_t closing = CodeTokenizer::match_unescaped(
                             { value.data() + opening, value.length() - opening }, is_closing) +
                         opening;

  return string(value.data() + opening, closing - opening);
}

CPreprocessor::Token CPreprocessor::_parse_preprocessor_line(source_t source) {
  LOG_ASSERT(source.current() == '#');
  source.skip();

  // skip white chars, not newlines + skip comments
  _try_skip_to_usable(source);

  // we expect a lowercase letter
  if (!string_tools::is_lowercase(source.current()))
  {
    return Token();
  }

  const string name = _read_preprocessor_name(source);

  // std::cout << "name: " << name << " - " << source.tell() << '\n';

  _try_skip_to_usable(source);

  // either we find a whitespace or a whitespace
  if (source.depleted() || source.current() == '\n')
  {
    return Token(name, "");
  }

  const string value = _read_preprocessor_value(source);

  // std::cout << "value: " << value << " - " << source.tell() << '\n';

  return Token(name, value);
}

string CPreprocessor::_read_preprocessor_name(source_t source) {
  string name = source.get_while(string_tools::is_lowercase);
  return name;
}

string CPreprocessor::_read_preprocessor_value(source_t source) {
  string value = _read_value(source);

  // trim whitespace end
  intptr_t index = intptr_t(value.size() - 1);
  for (; index >= 0; index--)
  {
    if (!string_tools::is_whitespace(value[index]))
    {
      break;
    }
  }

  value.resize(index + 1);

  return value;
}

string CPreprocessor::_read_value(source_t source) {
  string result = { 0 };

  bool in_string = false;
  while (!source.depleted())
  {
    const char cur = source.current();

    if (cur == '"')
    {
      in_string = !in_string;  // toggle it
    }

    if (in_string)
    {
      result.push_back(cur);
      source.skip(1);
      continue;
    }

    if (cur == '\n')
    {
      break;
    }

    _try_skip_comment(source);

    if (source.depleted())
    {
      break;
    }

    result.push_back(source.get());
  }

  return result;
}

void CPreprocessor::_try_skip_comment(source_t source) {
  if (source.available() < 2)
  {
    return;
  }

  if (source[0] != '/')
  {
    return;
  }

  // line comment
  if (source[1] == '/')
  {
    source.skip_until([](char cur) { return cur == '\n'; });
    return;  // if not depleted, pointing to a newline
  }

  // block comment
  if (source[1] == '*')
  {
    source.skip(2);

    while (!source.depleted())
    {
      if (source[0] == '*' && source[1] == '/')
      {
        source.skip(2);
        return;
      }

      source.skip(1);
    }

    // if not depleted, pointing to anything after '*/'
    return;
  }
}

void CPreprocessor::_try_skip_to_usable(source_t source) {
  source.skip_while(whitespace_no_newlines);
  _try_skip_comment(source);
  source.skip_while(whitespace_no_newlines);
}

string CPreprocessor::_escape(const StrBlob &source) {
  string result{};
  result.reserve(source.length());

  for (size_t i = 0; i < source.size; i++)
  {
    if (i + 1 < source.size && source[i] == '\\')
    {
      // skip escaped new lines
      if (source[i + 1] == '\n')
      {
        continue;
      }

      result.push_back(source[++i]);
      continue;
    }

    result.push_back(source[i]);
  }

  return result;
}

CPreprocessor::Token::Token(const string &name, const string &value)
    : type{ _get_tk_type({ name.c_str(), name.length() }) }, value{ value } {}
