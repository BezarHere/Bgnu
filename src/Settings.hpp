#pragma once
#include "Logger.hpp"
#include "FieldVar.hpp"

struct SettingValue {
	FieldVar default_value = nullptr;
	FieldVar value = nullptr;
};

struct Settings
{

	static errno_t Init();

	static const FieldVar &Get(const std::string &name, const FieldVar &default_val = {});
	static void Set(const std::string &name, const FieldVar &value);
	static const FieldVar &Reset(const std::string &name);

private:
	Settings() = delete;
	~Settings() = delete;
};

