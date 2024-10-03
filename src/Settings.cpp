#include "Settings.hpp"
#include "StringTools.hpp"
#include "FilePath.hpp"
#include "FieldFile.hpp"
#include "FieldDataReader.hpp"

typedef std::map<std::string, SettingValue> SettingMap;

static constexpr BGnuVersion BGnuVersionHistory[] = {
	{1, 0, 0},
	{1, 1, 0},
};

struct SettingsData
{
	static const SettingMap Default;

	bool initalized = false;
	SettingMap base_values;
	SettingMap custom_values;
};

static const SettingValue DefaultValue = {nullptr, nullptr};

SettingsData g_SettingsData;

static inline errno_t AddField(const std::string &name, const SettingValue &value, bool base_val = true);
static inline const FieldVar &GetSettingInnerValue(const std::string &name, const FieldVar &def_val);
static inline SettingValue *GetSettingValue(const std::string &name);

static inline void ReadSetting(FieldDataReader &reader);

static inline FilePath GetLocalSettingsPath();
static inline FieldVar LoadSettingsFile();

BGnuVersion Settings::GetVersion() {
	return BGnuVersionHistory[std::size(BGnuVersionHistory) - 1];
}

errno_t Settings::Init() {
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

const FieldVar &Settings::Get(const std::string &name, const FieldVar &default_val) {
	return GetSettingInnerValue(name, default_val);
}

void Settings::Set(const std::string &name, const FieldVar &value) {
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

	SettingMap &map = base_val ? g_SettingsData.base_values : g_SettingsData.custom_values;
	const auto name_processed = string_tools::to_lower(name);

	if (map.contains(name_processed))
	{
		return EALREADY;
	}

	map.emplace(name, value);
	return EOK;
}

inline const FieldVar &GetSettingInnerValue(const std::string &name, const FieldVar &def_val) {
	if (g_SettingsData.base_values.contains(name))
	{
		return g_SettingsData.base_values.at(name).value;
	}

	if (g_SettingsData.custom_values.contains(name))
	{
		return g_SettingsData.custom_values.at(name).value;
	}

	Logger::verbose("no setting field with the name '%s'", name.c_str());

	return def_val;
}

inline SettingValue *GetSettingValue(const std::string &name) {
	if (g_SettingsData.base_values.contains(name))
	{
		return &g_SettingsData.base_values.at(name);
	}

	if (g_SettingsData.custom_values.contains(name))
	{
		return &g_SettingsData.custom_values.at(name);
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
