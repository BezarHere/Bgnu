#pragma once
#include "Logger.hpp"
#include "FieldVar.hpp"

struct SettingValue {
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

	static errno_t Init();

	static const FieldVar &Get(const std::string &name, const FieldVar &default_val = {});
	static void Set(const std::string &name, const FieldVar &value);
	static const FieldVar &Reset(const std::string &name);

private:
	Settings() = delete;
	~Settings() = delete;
};

