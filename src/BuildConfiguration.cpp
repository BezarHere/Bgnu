#include "BuildConfiguration.hpp"
#include "StringUtils.hpp"
#include "FieldDataReader.hpp"
#include "Console.hpp"


BuildConfiguration BuildConfiguration::from_data(const FieldVar::Dict &data, ParsingResult &result) {
	static constexpr const char Context[] = "BuildConfiguration";
	BuildConfiguration config{};

	result = {};
	result.success = false;

	FieldDataReader reader{Context, data};


	{
		FieldVar predefines = reader.try_get_value<FieldVarType::Dict>("predefines");

		if (predefines.is_null())
		{
			result.reason = "no predefines";
			return config;
		}

		config.predefines = predefines.get_dict();

		for (auto &[key, value] : config.predefines) {
			if (value.is_null() || value.get_type() == FieldVarType::String)
			{
				continue;
			}

			Console::warning(
				("%s: predefine named '%s' is not a string or null,"
				 " the value will be stringified, but that may lead to undefined behavior"),
				Context, key.c_str()
			);

			value.stringify();
		}

	}


	{
		const FieldVar &var_optimization_level = \
			reader.try_get_value<FieldVarType::String>("optimization_lvl", get_enum_name(config.optimization.degree));

		OptimizationDegree optimization_level = get_optimization_degree(var_optimization_level);

		if (optimization_level == OptimizationDegree::None)
		{
			optimization_level = config.optimization.degree;
			Console::error(
				"%s: Invalid optimization level '%s', defaulting to '%s'",
				Context, var_optimization_level.get_string().c_str(), get_enum_name(optimization_level)
			);
		}

		const FieldVar &var_optimization_type = \
			reader.try_get_value<FieldVarType::String>("optimization_type", get_enum_name(config.optimization.type));

		OptimizationType optimization_type = get_optimization_type(var_optimization_level);

		if (optimization_type == OptimizationType::None)
		{
			optimization_type = config.optimization.type;
			Console::error(
				"%s: Invalid optimization type '%s', defaulting to '%s'",
				Context, var_optimization_type.get_string().c_str(), get_enum_name(optimization_type)
			);
		}

		config.optimization.degree = optimization_level;
		config.optimization.type = optimization_type;
	}



	result.success = true;
	return config;
}

#pragma region(enum names)

constexpr array<const char *, 5> OptimizationTypeNames = {
	"none",

	"release",
	"debug",

	"size",
	"speed",
};

constexpr array<const char *, 5> OptimizationDegreeNames = {
	"none",

	"low",
	"medium",
	"high",

	"extreme",
};

constexpr array<const char *, 4> WarningLevelNames = {
	"none",

	"few",
	"most",
	"all",
};

constexpr array<const char *, 13> StandardTypeNames = {
	"none",

	"C99",
	"C11",
	"C14",
	"C17",
	"C23",

	"Cpp99",
	"Cpp11",
	"Cpp14",
	"Cpp17",
	"Cpp20",
	"Cpp23",

	"latest",
};

constexpr array<const char *, 9> SIMDTypeNames = {
	"none",

	"SSE",
	"SSE2",
	"SSE3",
	"SSE3.1",
	"SSE4",

	"AVX",
	"AVX2",
	"AVX512",
};

const char *BuildConfiguration::get_enum_name(OptimizationType opt_type) {
	return OptimizationTypeNames[(int)opt_type];
}

const char *BuildConfiguration::get_enum_name(OptimizationDegree opt_degree) {
	return OptimizationDegreeNames[(int)opt_degree];
}

const char *BuildConfiguration::get_enum_name(WarningLevel wrn_lvl) {
	return WarningLevelNames[(int)wrn_lvl];
}

const char *BuildConfiguration::get_enum_name(StandardType standard) {
	return StandardTypeNames[(int)standard];
}

const char *BuildConfiguration::get_enum_name(SIMDType simd) {
	return SIMDTypeNames[(int)simd];
}

OptimizationType BuildConfiguration::get_optimization_type(const string &name) {

	for (size_t i = 1; i < OptimizationTypeNames.size(); ++i)
	{
		if (StringUtils::equal_insensitive(OptimizationTypeNames[i], name.c_str(), name.size()))
		{
			return OptimizationType(i);
		}
	}

	return OptimizationType::None;
}

OptimizationDegree BuildConfiguration::get_optimization_degree(const string &name) {

	for (size_t i = 1; i < OptimizationDegreeNames.size(); ++i)
	{
		if (StringUtils::equal_insensitive(OptimizationDegreeNames[i], name.c_str(), name.size()))
		{
			return OptimizationDegree(i);
		}
	}

	return OptimizationDegree::None;
}

WarningLevel BuildConfiguration::get_warning_level(const string &name) {

	for (size_t i = 1; i < WarningLevelNames.size(); ++i)
	{
		if (StringUtils::equal_insensitive(WarningLevelNames[i], name.c_str(), name.size()))
		{
			return WarningLevel(i);
		}
	}

	return WarningLevel::None;
}

StandardType BuildConfiguration::get_standard_type(const string &name) {

	for (size_t i = 1; i < StandardTypeNames.size(); ++i)
	{
		if (StringUtils::equal_insensitive(StandardTypeNames[i], name.c_str(), name.size()))
		{
			return StandardType(i);
		}
	}

	return StandardType::None;
}

SIMDType BuildConfiguration::get_simd_type(const string &name) {

	for (size_t i = 1; i < SIMDTypeNames.size(); ++i)
	{
		if (StringUtils::equal_insensitive(SIMDTypeNames[i], name.c_str(), name.size()))
		{
			return SIMDType(i);
		}
	}

	return SIMDType::None;
}

#pragma endregion
