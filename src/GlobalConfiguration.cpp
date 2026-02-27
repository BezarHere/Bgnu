#include "GlobalConfiguration.hpp"

#include <cctype>
#include <cstdlib>
#include <ostream>
#include <set>

#include "FilePath.hpp"
#include "Logger.hpp"
#include "utility/PPL.hpp"

std::map<std::string, float> GlobalConfiguration::s_numbers = {};
std::map<std::string, bool> GlobalConfiguration::s_flags = {};
std::map<std::string, std::string> GlobalConfiguration::s_strings = {};

template <typename T>
static void dump_values(std::set<std::string> &names,
                        const std::map<std::string, T> &map,
                        std::ostream &stream);

void GlobalConfiguration::Load() {
  if (!GetPath().is_file())
  {
    GetPath().stream_write(false) << "# THIS IS THE .BGNU GLOBAL CONFIGURATION FILE\n\n";
    return;
  }

  auto p = ppl::LoadFile(GetPath().c_str());
  if (p.has_error())
  {
    Logger::error(p.to_error_report());
    return;
  }
  for (const auto &s : p.value())
  {
    if (HasAnyValueNamed(s.name.c_str()))
    {
      continue;
    }
    AddValue(s.name.c_str(), s.value);
  }
}

void GlobalConfiguration::Save() {
  std::set<std::string> saved_names{};
  auto stream = GetPath().stream_write(false);
  dump_values(saved_names, s_numbers, stream);
  dump_values(saved_names, s_flags, stream);
  dump_values(saved_names, s_strings, stream);
}

bool GlobalConfiguration::HasAnyValueNamed(const char *name) {
  return GetValueType(name) != ValueType::None;
}

auto GlobalConfiguration::GetValueType(const char *name) -> ValueType {
  if (s_flags.contains(name))
  {
    return ValueType::Flag;
  }
  if (s_numbers.contains(name))
  {
    return ValueType::Number;
  }
  if (s_strings.contains(name))
  {
    return ValueType::String;
  }

  return ValueType::None;
}

FilePath GlobalConfiguration::GetPath() { return FilePath("~/.bgnu-config").resolved_copy(); }

void GlobalConfiguration::AddValue(const char *name, const std::string &value) {
  if (value == "false" || value == "true")
  {
    s_flags.emplace(name, value[0] == 't');
    Logger::debug("Added a boolean %s to the global config", name);
    return;
  }

  if (value[0] == '"' && value.back() == '"')
  {
    s_strings.emplace(name, value.substr(1, value.size() - 2));
    Logger::debug("Added a quoted string %s to the global config", name);
    return;
  }

  const char *value_end = value.c_str() + value.size();

  char *end_ptr = const_cast<char *>(value_end);
  const float value_f = strtof(value.c_str(), &end_ptr);

  if (end_ptr == value_end)
  {
    s_numbers.emplace(name, value_f);
    Logger::debug("Added a number %s to the global config", name);
    return;
  }

  s_strings.emplace(name, value);
  Logger::debug("Added a string %s to the global config", name);
}

template <typename T>
void dump_values(std::set<std::string> &names,
                 const std::map<std::string, T> &map,
                 std::ostream &stream) {
  for (const auto &[name, val] : map)
  {
    if (names.contains(name))
    {
      continue;
    }
    names.emplace(name);
    stream << name << "= " << val << '\n';
  }
}
