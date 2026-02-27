#pragma once

// Property Per Line
#include <string>
#include <vector>

#include "Result.hpp"
#include "misc/StaticString.hpp"
namespace ppl
{
  constexpr size_t PropertyNameSize = 256;
  struct PropertyLine
  {
    StaticString<PropertyNameSize> name;
    std::string value;
    int line;
  };

  using PropertyList = std::vector<PropertyLine>;

  extern Result<PropertyList> LoadFile(const char *filepath);
  extern Result<PropertyList> TryParseContent(const char *str, size_t len);
  extern Result<PropertyLine> TryParseLine(const char *str, size_t len);

  extern std::vector<std::string> ToLines(const std::string &str);
}
