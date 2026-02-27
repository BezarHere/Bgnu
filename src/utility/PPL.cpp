#include "PPL.hpp"

#include <cctype>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include "Console.hpp"
#include "Result.hpp"
#include "StringTools.hpp"
#include "base.hpp"

using namespace ppl;

constexpr char CommentChar = '#';

static StrBlob TrimComment(StrBlob blob);
static StrBlob TrimEnds(StrBlob blob);
static std::string ReadName(StrBlob &blob);
static std::string ReadValue(StrBlob &blob);
static bool ReadAssignment(StrBlob &blob);
static bool IsValidNameChar(char chr);

Result<PropertyList> ppl::LoadFile(const char *filepath) {
  FILE *fp = fopen(filepath, "r");
  if (fp == NULL)
  {
    return Error::FileNotFound;
  }

  const long start = ftell(fp);
  fseek(fp, 0, SEEK_END);
  const long end = ftell(fp);
  rewind(fp);

  const long size = end - start;
  std::string buf{};
  buf.resize(size);

  fread(buf.data(), buf.size(), 1, fp);
  fclose(fp);

  return TryParseContent(buf.data(), buf.size());
}

Result<PropertyList> ppl::TryParseContent(const char *str, size_t len) {
  size_t last_index = 0;
  int current_line = 0;
  std::vector<std::pair<int, StrBlob>> blobs{};
  for (size_t i = 0; i <= len; i++)
  {
    if (str[i] != '\n' && str[i] != '\r' && i < len)
    {
      continue;
    }

    if (str[i] == '\n')
    {
      current_line++;
    }

    if (last_index == i)
    {
      last_index++;
      continue;
    }

    blobs.emplace_back(current_line, StrBlob{ str + last_index, i - last_index });
    last_index = i + 1;
  }

  PropertyList list{};
  for (auto [line, blob] : blobs)
  {
    blob = TrimEnds(blob);
    if (blob.empty())
    {
      continue;
    }

    Result<PropertyLine> property = TryParseLine(blob.data, blob.length);
    if (property.has_error())
    {
      if (property.error() == Error::NoData)
      {
        continue;
      }

      return { property.error(), format_join("At line ", line, ": ", property.message()) };
    }

    list.push_back(property.value());
    list.back().line = line;
  }

  return list;
}

Result<PropertyLine> ppl::TryParseLine(const char *str, size_t len) {
  StrBlob blob = { str, len };

  blob = TrimComment(blob);
  blob = TrimEnds(blob);

  if (blob.empty())
  {
    return { Error::NoData, "No data" };
  }

  std::string name = ReadName(blob);
  if (name.empty())
  {
    return { Error::Failure, "No valid name" };
  }

  if (blob.empty())
  {
    return { Error::Failure, "Expecting value after name" };
  }

  if (!ReadAssignment(blob))
  {
    return { Error::Failure, format_join("Expecting '=' after name, found '", blob[0], "'") };
  }

  std::string value = ReadValue(blob);

  return PropertyLine{ name, value, -1 };
}

StrBlob TrimComment(StrBlob blob) {
  for (size_t i = 0; i < blob.length; i++)
  {
    if (blob[i] == CommentChar)
    {
      return { blob.data, i };
    }
  }

  return blob;
}

StrBlob TrimEnds(StrBlob blob) {
  while (blob.length > 0 && std::isspace(*blob.data))
  {
    blob.data++;
    blob.length--;
  }

  while (blob.length > 0 && std::isspace(blob.end()[-1]))
  {
    blob.length--;
  }

  return blob;
}

std::string ReadName(StrBlob &blob) {
  blob = TrimEnds(blob);

  size_t i;
  for (i = 0; i < blob.length; i++)
  {
    if (!IsValidNameChar(blob[i]))
    {
      break;
    }
  }

  if (i == 0)
  {
    return "";
  }

  const char *result_start = blob.data;
  blob.shift_begin(i);

  return { result_start, i };
}

std::string ReadValue(StrBlob &blob) {
  blob = TrimEnds(blob);
  const std::string str = { blob.data, blob.length };
  blob.shift_begin(blob.length);
  return str;
}

bool ReadAssignment(StrBlob &blob) {
  blob = TrimEnds(blob);
  if (blob[0] == '=')
  {
    blob.shift_begin(1);
    return true;
  }
  return false;
}

bool IsValidNameChar(char chr) { return std::isalpha(chr) || chr == '_' || std::isdigit(chr); }
