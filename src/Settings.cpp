#include "Settings.hpp"
#include "StringTools.hpp"
#include "FilePath.hpp"
#include "FieldFile.hpp"
#include "FieldDataReader.hpp"

typedef std::map<std::string, SettingValue> SettingMap;

static constexpr BGnuVersion BGnuVersionHistory[] = {
  {1, 0, 0}, {1, 1, 0},
  {1, 2, 0}, {1, 2, 1}, {1, 2, 2}, {1, 2, 3}, {1, 2, 4},
  {1, 3, 0}, {1, 3, 1}, {1, 3, 2}, {1, 3, 3},
  {1, 4, 0}, {1, 4, 1}, {1, 4, 2}, {1, 4, 3}, {1, 4, 4}, {1, 4, 5},
  {1, 5, 0}, {1, 5, 1}, {1, 5, 2}
};

struct SettingsData
{
  static const SettingMap Default;

  bool initalized = false;
  SettingMap values;
};

static const SettingValue DefaultValue = {nullptr, nullptr};

SettingsData g_SettingsData = {0};

bool Settings::s_SilentSaveFail = false;
bool Settings::s_AddRequestValues = true;

static inline errno_t AddField(const std::string &name, const SettingValue &value, bool base_val = true);
static inline const FieldVar &GetSettingInnerValue(const std::string &name, const FieldVar &def_val);
static inline SettingValue *GetSettingValue(const std::string &name);

static inline void ReadSetting(FieldDataReader &reader);

static inline FilePath GetLocalSettingsPath();
static inline FieldVar LoadSettingsFile();

static FieldVar::Dict Serialize();

BGnuVersion Settings::GetVersion() {
  return BGnuVersionHistory[std::size(BGnuVersionHistory) - 1];
}

errno_t Settings::Init(ArgumentSource &args) {
  // TODO: handle global flags
  (void)args;
  
  if (g_SettingsData.initalized)
  {
    return EALREADY;
  }
  g_SettingsData.initalized = true;

  Logger::note("initialising settings");

  const FieldVar local_settings = LoadSettingsFile();
  FieldDataReader reader{"local-settings", local_settings.get_dict()};
  ReadSetting(reader);

  return EOK;
}

errno_t Settings::Save(bool overwrite) {
  return SaveTo(GetLocalSettingsPath(), overwrite);
}

errno_t Settings::SaveBackup() {
  const FilePath backup_path = GetLocalSettingsPath() + ".backup";
  Logger::verbose("backing up settings to '%s'", backup_path.c_str());
  return SaveTo(backup_path, true);
}

errno_t Settings::SaveTo(const FilePath &path, bool overwrite) {
  if (!overwrite && path.is_file())
  {
    if (s_SilentSaveFail)
    {
      return EOK;
    }

    Logger::error("Can not save setting locals file to '%s', file already exists", path.c_str());
    return EALREADY;
  }

  FieldFile::dump(path, Serialize());
  return EOK;
}

const FieldVar &Settings::Get(const std::string &name, const FieldVar &default_val) {
  return GetSettingInnerValue(name, default_val);
}

void Settings::Set(const std::string &name, const FieldVar &value) {
  throw std::runtime_error("unimplemented");
}

const FieldVar &Settings::Reset(const std::string &name) {
  SettingValue *value = GetSettingValue(name);

  if (value == nullptr)
  {
    return DefaultValue.value;
  }

  value->value = value->default_value;
  return value->value;
}

inline errno_t AddField(const std::string &name, const SettingValue &value, bool base_val) {
  if (name.empty())
  {
    return EBADF;
  }

  SettingMap &map = g_SettingsData.values;
  const auto name_processed = string_tools::to_lower(name);

  if (map.contains(name_processed))
  {
    return EALREADY;
  }

  map.emplace(name, value);
  return EOK;
}

inline const FieldVar &GetSettingInnerValue(const std::string &name, const FieldVar &def_val) {
  if (g_SettingsData.values.contains(name))
  {
    const FieldVar &value = g_SettingsData.values.at(name).value;
    if (Logger::is_verbose())
    {
      const std::string out = value.copy_stringified().get_string();
      Logger::verbose(
        "returned setting field value name='%s', value=%s",
        name.c_str(), out.c_str()
      );
    }
    return value;
  }

  Logger::verbose("no setting field with the name '%s'", name.c_str());

  if (Settings::s_AddRequestValues)
  {
    Logger::verbose("add default setting field '%s'", name.c_str());
    SettingValue value = {};
    value.value = def_val;
    value.default_value = def_val;
    AddField(name, value);
  }


  return def_val;
}

inline SettingValue *GetSettingValue(const std::string &name) {
  if (g_SettingsData.values.contains(name))
  {
    return &g_SettingsData.values.at(name);
  }

  return nullptr;
}

inline void ReadSetting(FieldDataReader &reader) {
  for (const auto &[name, value] : reader.get_data())
  {
    const errno_t err = AddField(name, {value, value});
    if (err == EOK)
    {
      Logger::debug("added setting: %s", name.c_str());
      continue;
    }

    Logger::warning("failed to add setting field '%s', error=%s", name.c_str(), GetErrorName(err));
  }
}

inline FilePath GetLocalSettingsPath() {
  return FilePath::get_working_directory() + "settings.bgnu";
}

inline FieldVar LoadSettingsFile() {
  return FieldFile::load(GetLocalSettingsPath());
}

FieldVar::Dict Serialize() {
  FieldVar::Dict dict = {};

  for (const auto &[name, value] : g_SettingsData.values)
  {
    dict.try_emplace(name, value.value);
  }

  return dict;
}
