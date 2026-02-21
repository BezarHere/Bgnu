#pragma once
#include "Argument.hpp"
#include "FieldVar.hpp"
#include "FilePath.hpp"
#include "Logger.hpp"

struct SettingValue
{
  FieldVar value = nullptr;
  FieldVar default_value = nullptr;
};

struct BGnuVersion
{
  uint8_t major;
  uint8_t minor;
  int16_t patch;
};

struct Settings
{
public:
  static BGnuVersion GetVersion();

  static errno_t Init(ArgumentSource &args);
  static errno_t Save(bool overwrite);
  static errno_t SaveBackup();
  static errno_t SaveTo(const FilePath &path, bool overwrite);

  static const FieldVar &Get(const std::string &name, const FieldVar &default_val = {});
  static void Set(const std::string &name, const FieldVar &value);
  static const FieldVar &Reset(const std::string &name);

  // defaults to false
  static bool s_SilentSaveFail;

  // defaults to true, when getting a value, adds the value if it's not present (to the default)
  static bool s_AddRequestValues;

private:
  Settings() = delete;
  ~Settings() = delete;
};

namespace std
{

  inline ostream &operator<<(ostream &stream, BGnuVersion version) {
    return stream << int(version.major) << '.' << int(version.minor) << '.' << int(version.patch);
  }

}
