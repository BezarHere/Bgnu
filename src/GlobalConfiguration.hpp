#pragma once

#include <map>
#include <string>

#include "FilePath.hpp"

class GlobalConfiguration
{
public:
  enum class ValueType { None, Number, Flag, String };

  static void Load();
  static void Save();

  static bool HasAnyValueNamed(const char *name);
  static ValueType GetValueType(const char *name);

  static FilePath GetPath();

private:
  static void AddValue(const char *name, const std::string &value);

private:
  static std::map<std::string, float> s_numbers;
  static std::map<std::string, bool> s_flags;
  static std::map<std::string, std::string> s_strings;
};
