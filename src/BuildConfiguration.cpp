#include "BuildConfiguration.hpp"
#include "StringUtils.hpp"
#include "FieldDataReader.hpp"
#include "Console.hpp"

static inline constexpr const char *to_cstr(const char *value) { return value; }
static inline const char *to_cstr(const string &value) { return value.c_str(); }

template <typename Type, typename ParseProc, typename StringifyProc, typename ValidateProc>
struct EnumTraits
{
	using enum_type = Type;
	using string_type = FieldVar::String;

	string_type _name;
	enum_type default_value;

	const ParseProc parse_proc;
	const StringifyProc stringify_proc;
	const ValidateProc validate_proc;

	EnumTraits(const string_type &name, Type _default_val,
						 ParseProc &&_parse_proc,
						 StringifyProc &&_stringify_proc,
						 ValidateProc &&_validate_proc)
		: _name{name}, default_value{_default_val},
		parse_proc{_parse_proc}, stringify_proc{_stringify_proc}, validate_proc{_validate_proc} {

	}

	ALWAYS_INLINE enum_type parse(const string_type &str) const { return parse_proc(str); }
	ALWAYS_INLINE string_type stringify(enum_type value) const { return stringify_proc(value); }
	ALWAYS_INLINE bool validate(enum_type value) const { return validate_proc(value); }

	ALWAYS_INLINE enum_type get_default() const { return default_value; }
	ALWAYS_INLINE string_type get_default_named() const { return stringify(default_value); }
	ALWAYS_INLINE enum_type name() const { return _name; }
};

struct BuildConfigurationReader
{
	BuildConfigurationReader(const BuildConfiguration &_config, FieldDataReader &_reader, ErrorReport &_result)
		: config{_config}, reader{_reader}, result{_result} {
	}

	void read_predefines(FieldVar::Dict &predefines);
	void read_optimization_info(OptimizationInfo &info);
	void read_warning_info(WarningReportInfo &info);

	void read_strings_named(const string &name, vector<string> &out);

	template <typename _Traits>
	inline typename _Traits::enum_type _read_enum(const string &name, const _Traits &traits) {

		const FieldVar &enum_field_var = \
			reader.try_get_value<FieldVarType::String>(name);

		if (enum_field_var.is_null())
		{
			Console::error(
				"%s: Value for %s ('%s') isn't defined, defaulting to '%s'",
				to_cstr(reader._context),
				to_cstr(traits.name()),
				to_cstr(name),
				to_cstr(traits.get_default_named())
			);
			return traits.get_default();
		}

		typename _Traits::enum_type enum_val = traits.parse(enum_field_var);

		if (!traits.validate(enum_val))
		{
			enum_val = traits.get_default();
			Console::error(
				"%s: Invalid %s value '%s', defaulting to '%s'",
				to_cstr(reader._context),
				to_cstr(traits.name()),
				enum_field_var.get_string().c_str(),
				to_cstr(traits.get_default_named())
			);
		}

		return enum_val;
	}

	const BuildConfiguration &config;
	FieldDataReader &reader;
	ErrorReport &result;
};


#define CHECK_RESULT(expr) expr; if (result.code != Error::Ok) { return config; }
BuildConfiguration BuildConfiguration::from_data(const FieldVar::Dict &data, ErrorReport &result) {
	static constexpr const char Context[] = "BuildConfiguration";
	BuildConfiguration config{};

	result = {};
	result.code = Error::Ok;

	FieldDataReader reader{Context, data};

	BuildConfigurationReader bc_reader{config, reader, result};


	CHECK_RESULT(bc_reader.read_predefines(config.predefines));

	CHECK_RESULT(bc_reader.read_optimization_info(config.optimization));
	CHECK_RESULT(bc_reader.read_warning_info(config.warnings));

	typedef inner::NamedValue<bool *, string> NamedBool;

	const array<NamedBool, 4> booleans = {
		NamedBool{"exit_on_errors", &config.exit_on_errors},
		NamedBool{"print_stats", &config.print_stats},
		NamedBool{"print_includes", &config.print_includes},
		NamedBool{"dynamically_linkable", &config.dynamically_linkable},
	};

	for (const auto &bool_ptr : booleans)
	{
		const FieldVar &var = reader.try_get_value<FieldVarType::Boolean>(bool_ptr.name);
		if (var.is_null())
		{
			continue;
		}

		*bool_ptr.value = var.get_bool();
	}

	CHECK_RESULT(
		bc_reader.read_strings_named("preprocessor_args", config.preprocessor_args);
	);
	CHECK_RESULT(
		bc_reader.read_strings_named("linker_args", config.linker_args);
	);
	CHECK_RESULT(
		bc_reader.read_strings_named("assembler_args", config.assembler_args);
	);

	CHECK_RESULT(
		bc_reader.read_strings_named("lib_names", config.library_names);
	);

	return config;
}

void BuildConfigurationReader::read_predefines(FieldVar::Dict &predefines) {
	const FieldVar &field_var = reader.try_get_value<FieldVarType::Dict>("predefines");

	if (field_var.is_null())
	{
		result.code = Error::NoData;
		result.message = "no predefines";
		return;
	}

	for (const auto &[key, value] : field_var.get_dict()) {
		if (value.is_null() || value.get_type() == FieldVarType::String)
		{
			predefines.insert_or_assign(key, value);
			continue;
		}

		Console::warning(
			("%s: predefine named '%s' is not a string or null,"
			 " the value will be stringified, but that may lead to undefined behavior"),
			to_cstr(reader._context), key.c_str()
		);

		predefines.insert_or_assign(key, value.copy_stringified());
	}

}

template <typename T>
using GetEnumNameOfType = const char *(*)(T);

void BuildConfigurationReader::read_optimization_info(OptimizationInfo &info) {

	EnumTraits opt_type{
		"optimization type",
		info.type,
		BuildConfiguration::get_optimization_type,
		(GetEnumNameOfType<OptimizationType>)BuildConfiguration::get_enum_name,
		[](const OptimizationType type) { return type != OptimizationType::None; }
	};

	EnumTraits opt_level{
		"optimization level",
		info.degree,
		BuildConfiguration::get_optimization_degree,
		(GetEnumNameOfType<OptimizationDegree>)BuildConfiguration::get_enum_name,
		[](const OptimizationDegree type) { return type != OptimizationDegree::None; }
	};

	info.type = _read_enum("optimization_type", opt_type);
	info.degree = _read_enum("optimization_lvl", opt_level);
}

void BuildConfigurationReader::read_warning_info(WarningReportInfo &info) {
	EnumTraits wrn_type{
		"warning level",
		info.level,
		BuildConfiguration::get_warning_level,
		(GetEnumNameOfType<WarningLevel>)BuildConfiguration::get_enum_name,
		[](const WarningLevel type) { return type != WarningLevel::None; }
	};

	info.level = _read_enum("warning_level", wrn_type);
}

void BuildConfigurationReader::read_strings_named(const string &name, vector<string> &out) {
	const FieldVar &var = reader.try_get_value<FieldVarType::Array>(name);

	if (var.is_null())
	{
		result.code = Error::NoData;
		result.message = format_join("required string array named \"", name, "\" do not exist");
		return;
	}

	const auto &array = var.get_array();

	for (size_t i = 0; i < array.size(); ++i)
	{
		if (array[i].is_convertible_to(FieldVarType::String))
		{
			out.push_back(array[i].get_string());
			continue;
		}

		if (array[i].is_simple_type())
		{
			out.push_back(array[i].copy_stringified().get_string());
			continue;
		}

		result.code = Error::InvalidType;
		result.message = format_join(
			"value at \'",
			name,
			'[',
			i,
			"] should be a string, but it's of type '",
			FieldVar::get_type_name(array[i].get_type()),
			'\''
		);

		auto err_msg = reader._context + ':' + result.message;
		Console::error(
			err_msg.c_str()
		);
		break;
	}

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



