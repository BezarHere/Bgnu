#include "BuildConfiguration.hpp"
#include "StringTools.hpp"
#include "FieldDataReader.hpp"
#include "Logger.hpp"

#include <algorithm>
#include "Settings.hpp"

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
	ALWAYS_INLINE const string_type &name() const { return _name; }
};

static constexpr StandardType transform_standards(StandardType original, SourceFileType source_type);

struct BuildConfigurationReader
{
	BuildConfigurationReader(const BuildConfiguration &_config, FieldDataReader &_reader, ErrorReport &_result)
		: config{_config}, reader{_reader}, result{_result} {
	}

	void read_predefines(FieldVar::Dict &predefines);
	void read_optimization_info(OptimizationInfo &info);
	void read_warning_info(WarningReportInfo &info);

	void read_strings_named(const string &name, vector<string> &out);
	void read_paths_named(const string &name, vector<FilePath> &out);

	template <typename _Traits>
	inline typename _Traits::enum_type _read_enum(const string &name, const _Traits &traits) {

		const FieldVar &enum_field_var = \
			reader.try_get_value<FieldVarType::String>(name);

		if (enum_field_var.is_null())
		{
			Logger::error(
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
			const auto default_named = traits.get_default_named();

			enum_val = traits.get_default();
			Logger::error(
				"%s: Invalid %s value '%s', defaulting to '%s'",
				to_cstr(reader._context),
				to_cstr(traits.name()),
				enum_field_var.get_string().c_str(),
				to_cstr(default_named)
			);
		}

		return enum_val;
	}

	const BuildConfiguration &config;
	FieldDataReader &reader;
	ErrorReport &result;
};

void BuildConfiguration::_put_predefines(vector<string> &output) const {
	for (const auto &[name, value] : predefines)
	{
		if (value.is_null())
		{
			output.emplace_back("-D");
			output.back().append(name);
			continue;
		}

		const bool valid_string = \
			value.is_convertible_to(FieldVarType::String)
			|| value.is_type_simple(FieldVarType::String);

		if (!valid_string)
		{
			Logger::error(
				"Invalid predefine value named \"%s\" of type %s, skipping it...",
				to_cstr(name),
				value.get_type_name()
			);

			continue;
		}

		if (value.get_type() != FieldVarType::String)
		{
			output.emplace_back("-D");
			output.back().append(name);
			output.back().append("=\"");

			// FIXME: escape the value before appending
			output.back().append(value.copy_stringified().get_string());

			output.back().append("\"");
		}
	}
}

void BuildConfiguration::_put_flags(vector<string> &output) const {
	if (this->exit_on_errors)
	{
		output.emplace_back("-Wfatal-errors");
	}

	if (Settings::Get("color_output", true))
	{
		output.emplace_back("-fdiagnostics-color=always");
	}

}

void BuildConfiguration::_put_standards(vector<string> &output, SourceFileType type) const {
	StandardType standard_type = transform_standards(standard, type);

	output.emplace_back("-std=");
	output.back().append(get_enum_name(standard_type));
}

void BuildConfiguration::_put_optimization(vector<string> &output) const {

	switch (optimization.type)
	{
	case OptimizationType::None:
		output.emplace_back("-O0");
		return;

	case OptimizationType::Release:
		output.emplace_back("-O");
		output.back().append(1, '0' + (int)optimization.degree);
		return;

	case OptimizationType::Debug:
		{
			output.emplace_back("-Og");

			output.emplace_back("-g");
			if (optimization.degree != OptimizationDegree::None)
			{
				output.back().append(1, '0' + (int)optimization.degree);
			}

			return;
		}

	case OptimizationType::Size:
		{
			if ((int)optimization.degree >= (int)OptimizationDegree::High)
			{
				output.emplace_back("-Oz");
				return;
			}

			output.emplace_back("-Os");
			return;
		}

	case OptimizationType::SpeedUnreliable:
		{
			output.emplace_back("-Ofast");
			return;
		}

	default:
		return;
	}
}

void BuildConfiguration::_put_warnings(vector<string> &output) const {
	switch (warnings.level)
	{
	case WarningLevel::None:
		{
			break;
		}
	case WarningLevel::All:
		{
			output.emplace_back("-Wall");
			break;
		}
	case WarningLevel::Extra:
		{
			output.emplace_back("-Wextra");
			break;
		}
	}


	if (warnings.pedantic)
	{
		output.emplace_back("-Wpedantic");
	}
}

void BuildConfiguration::_put_misc(vector<string> &output) const {
	if (simd_type == SIMDType::None)
	{
		return;
	}

	output.emplace_back("-m");
	output.back().append(string_tools::to_lower(get_enum_name(simd_type)));
}

void BuildConfiguration::_put_includes(vector<string> &output) const {
	for (const auto &include_dir : include_directories)
	{
		output.emplace_back("-I");

		output.emplace_back();
		output.back().append("\"");
		output.back().append(include_dir);
		output.back().append("\"");
	}
}

void BuildConfiguration::_put_libraries(vector<string> &output) const {
	for (const auto &lib_dir : library_directories)
	{
		output.emplace_back("-L");

		output.emplace_back();
		output.back().append("\"");
		output.back().append(lib_dir);
		output.back().append("\"");
	}

	for (const auto &lib_dir : library_names)
	{
		output.emplace_back("-l");

		output.emplace_back();
		output.back().append("\"");
		output.back().append(lib_dir);
		output.back().append("\"");
	}
}

void BuildConfiguration::build_arguments(vector<string> &output,
																				 const StrBlob &input_file, const StrBlob &output_file,
																				 SourceFileType type) const {
	_put_predefines(output);

	_put_optimization(output);
	_put_standards(output, type);
	_put_warnings(output);
	_put_flags(output);
	_put_misc(output);

	_put_includes(output);

	output.emplace_back("-c");

	output.emplace_back("\"");
	output.back().append(input_file.begin(), input_file.length());
	output.back().append("\"");

	output.emplace_back("-o");

	output.emplace_back("\"");
	output.back().append(output_file.begin(), output_file.length());
	output.back().append("\"");

	_put_libraries(output);
}

void BuildConfiguration::build_link_arguments(vector<string> &output,
																							const Blob<const StrBlob> &files,
																							const StrBlob &ouput_file) const {
	_put_predefines(output);

	_put_optimization(output);
	_put_standards(output, SourceFileType::None);
	_put_warnings(output);
	_put_misc(output);

	_put_includes(output);

	for (const StrBlob &blob : files)
	{
		output.emplace_back(1, '"');
		output.back().append(blob.begin(), blob.length());
		output.back().append(1, '"');
	}
		
	output.emplace_back("-o");


	output.emplace_back(1, '"');
	output.back().append(ouput_file.begin(), ouput_file.length());
	output.back().append(1, '"');

	_put_libraries(output);
}

hash_t BuildConfiguration::hash() const {
	HashDigester digester{};

	digester += FieldVar::hash_dict(this->predefines);
	digester += optimization;
	digester += warnings;

	digester += (hash_t)this->standard;

	digester += (hash_t)this->exit_on_errors;
	digester += (hash_t)this->print_stats;
	digester += (hash_t)this->print_includes;
	digester += (hash_t)this->dynamically_linkable;

	digester += (hash_t)this->simd_type;


	std::for_each(
		preprocessor_args.begin(), preprocessor_args.end(),
		[&digester](const string &str) { digester.add(str.c_str(), str.length()); }
	);
	std::for_each(
		linker_args.begin(), linker_args.end(),
		[&digester](const string &str) { digester.add(str.c_str(), str.length()); }
	);
	std::for_each(
		assembler_args.begin(), assembler_args.end(),
		[&digester](const string &str) { digester.add(str.c_str(), str.length()); }
	);

	std::for_each(
		library_names.begin(), library_names.end(),
		[&digester](const string &str) { digester.add(str.c_str(), str.length()); }
	);


	std::for_each(
		library_directories.begin(), library_directories.end(),
		[&digester](const FilePath &file_path) { digester.add(file_path.get_text()); }
	);

	std::for_each(
		include_directories.begin(), include_directories.end(),
		[&digester](const FilePath &file_path) { digester.add(file_path.get_text()); }
	);

	return digester.value;
}

// used in from_data()
#define CHECK_REPORT(expr) expr; if (report.code != Error::Ok) { return config; }

BuildConfiguration BuildConfiguration::from_data(FieldDataReader reader, ErrorReport &report) {
	BuildConfiguration config{};

	BuildConfigurationReader bc_reader{config, reader, report};


	CHECK_REPORT(bc_reader.read_predefines(config.predefines));

	CHECK_REPORT(bc_reader.read_optimization_info(config.optimization));
	CHECK_REPORT(bc_reader.read_warning_info(config.warnings));

	{
		EnumTraits standard{
		"standard version",
		config.standard,
		[](auto val) { return BuildConfiguration::get_standard_type(val); },
		[](StandardType type) { return BuildConfiguration::get_enum_name(type); },
		[](const StandardType type) { return type != StandardType::None; }
		};
		config.standard = bc_reader._read_enum("standard", standard);

		EnumTraits simd_type{
			"SIMD type",
			config.simd_type,
			[](auto val) { return BuildConfiguration::get_simd_type(val); },
			[](SIMDType type) { return BuildConfiguration::get_enum_name(type); },
			[](const SIMDType type) { return type != SIMDType::None; }
		};

		config.simd_type = bc_reader._read_enum("simd_type", simd_type);
	}

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

	CHECK_REPORT(
		bc_reader.read_strings_named("preprocessor_args", config.preprocessor_args);
	);
	CHECK_REPORT(
		bc_reader.read_strings_named("linker_args", config.linker_args);
	);
	CHECK_REPORT(
		bc_reader.read_strings_named("assembler_args", config.assembler_args);
	);

	CHECK_REPORT(
		bc_reader.read_strings_named("lib_names", config.library_names);
	);


	CHECK_REPORT(
		bc_reader.read_paths_named("lib_dirs", config.library_directories);
	);

	CHECK_REPORT(
		bc_reader.read_paths_named("include_dirs", config.include_directories);
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

		Logger::warning(
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
		[](auto val) { return BuildConfiguration::get_optimization_type(val); },
		[](OptimizationType type) { return BuildConfiguration::get_enum_name(type); },
		[](const OptimizationType type) { return type != OptimizationType::None; }
	};

	EnumTraits opt_level{
		"optimization level",
		info.degree,
		[](auto val) { return BuildConfiguration::get_optimization_degree(val); },
		[](OptimizationDegree degree) { return BuildConfiguration::get_enum_name(degree); },
		[](const OptimizationDegree type) { return type != OptimizationDegree::None; }
	};

	info.type = _read_enum("optimization_type", opt_type);
	info.degree = _read_enum("optimization_lvl", opt_level);
}

void BuildConfigurationReader::read_warning_info(WarningReportInfo &info) {
	EnumTraits wrn_type{
		"warning level",
		info.level,
		[](auto val) { return BuildConfiguration::get_warning_level(val); },
		[](WarningLevel lvl) { return BuildConfiguration::get_enum_name(lvl); },
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

		if (array[i].has_simple_type())
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
			FieldVar::get_name_for_type(array[i].get_type()),
			'\''
		);

		auto err_msg = reader._context + ':' + result.message;
		Logger::error(
			err_msg.c_str()
		);
		break;
	}

}

void BuildConfigurationReader::read_paths_named(const string &name, vector<FilePath> &out) {
	vector<string> paths_str{};
	read_strings_named(name, paths_str);

	for (const auto &path : paths_str)
	{
		out.emplace_back(path);
	}
}

#pragma region(enum names)

template <size_t MaxNameLn, size_t N>
struct NameList
{
	static constexpr string_char separator = 0;
	typedef string_char name_buf[MaxNameLn + 1];
	constexpr NameList() = default;
	template <size_t S>
	constexpr NameList(const string_char(&str)[S]) {
		size_t last = 0;
		for (size_t i = 0; i < S; i++)
		{
			if (str[i] == separator)
			{
				// duplicate separator
				if (i - last == 0)
				{
					last = i + 1;
					continue;
				}

				const size_t len = i - last;
				for (size_t j = 0; j < len; j++)
				{
					names[count][j] = str[i + last];
				}
				last = i + 1;
			}
		}

		if (S - last > 0)
		{
			const size_t len = S - last;

			for (size_t j = 0; j < len; j++)
			{
				names[count][j] = str[S + last];
			}
		}
	}

	const size_t count = 0;
	array<name_buf, N> names;
};

template <typename Enum>
struct NamedEnum
{
	constexpr NamedEnum(Enum _value, const string_char *_name)
		: name{_name}, value{_value} {
	}

	const string_char *name;
	Enum value;
};

constexpr NamedEnum<OptimizationType> OptimizationTypeNames[] = {
	NamedEnum(OptimizationType::None, "none"),

	NamedEnum(OptimizationType::Release, "release"),
	NamedEnum(OptimizationType::Release, "production"),
	NamedEnum(OptimizationType::Debug, "debug"),
	NamedEnum(OptimizationType::Debug, "testing"),

	NamedEnum(OptimizationType::Size, "size"),
	NamedEnum(OptimizationType::Size, "compress"),
	NamedEnum(OptimizationType::SpeedUnreliable, "speed"),
};

constexpr NamedEnum<OptimizationDegree> OptimizationDegreeNames[] = {
	NamedEnum(OptimizationDegree::None, "none"),
	NamedEnum(OptimizationDegree::None, "never"),

	NamedEnum(OptimizationDegree::Low, "low"),

	NamedEnum(OptimizationDegree::Medium, "medium"),
	NamedEnum(OptimizationDegree::Medium, "med"),
	NamedEnum(OptimizationDegree::Medium, "normal"),

	NamedEnum(OptimizationDegree::High, "high"),

	NamedEnum(OptimizationDegree::Extreme, "extreme"),
};

constexpr NamedEnum<WarningLevel> WarningLevelNames[] = {
	NamedEnum(WarningLevel::None, "none"),
	NamedEnum(WarningLevel::All, "all"),
	NamedEnum(WarningLevel::Extra, "extra"),
};

constexpr NamedEnum<StandardType> StandardTypeNames[] = {
	NamedEnum{StandardType::None, "none"},

	NamedEnum{StandardType::C99, "c99"},
	NamedEnum{StandardType::C11, "c11"},
	NamedEnum{StandardType::C14, "c14"},
	NamedEnum{StandardType::C17, "c17"},
	NamedEnum{StandardType::C23, "c23"},

	NamedEnum{StandardType::Cpp11, "c++11"},
	NamedEnum{StandardType::Cpp14, "c++14"},
	NamedEnum{StandardType::Cpp17, "c++17"},
	NamedEnum{StandardType::Cpp20, "c++20"},
	NamedEnum{StandardType::Cpp23, "c++23"},

	NamedEnum{StandardType::Cpp11, "cpp11"},
	NamedEnum{StandardType::Cpp14, "cpp14"},
	NamedEnum{StandardType::Cpp17, "cpp17"},
	NamedEnum{StandardType::Cpp20, "cpp20"},
	NamedEnum{StandardType::Cpp23, "cpp23"},

	NamedEnum{StandardType::Cpp23, "cpp2x"},
	NamedEnum{StandardType::Cpp23, "c++2x"},

	NamedEnum{StandardType::C99, "STD99"},
	NamedEnum{StandardType::Cpp11, "STD11"},
	NamedEnum{StandardType::Cpp14, "STD14"},
	NamedEnum{StandardType::Cpp17, "STD17"},
	NamedEnum{StandardType::Cpp23, "STD23"},

	NamedEnum{StandardType::Latest, "latest"},
};

constexpr NamedEnum<SIMDType> SIMDTypeNames[] = {
	NamedEnum(SIMDType::None, "none"),

	NamedEnum(SIMDType::SSE2, "SSE"),
	NamedEnum(SIMDType::SSE2, "SSE2"),
	NamedEnum(SIMDType::SSE3, "SSE3"),
	NamedEnum(SIMDType::SSE3_1, "SSE3.1"),
	NamedEnum(SIMDType::SSE4, "SSE4"),

	NamedEnum(SIMDType::AVX, "AVX"),
	NamedEnum(SIMDType::AVX2, "AVX2"),
	NamedEnum(SIMDType::AVX512, "AVX512"),
};

template <typename _T, size_t N>
inline constexpr const string_char *_find_enum_name(const NamedEnum<_T>(&names)[N], _T value) {
	for (size_t i = 0; i < N; i++)
	{
		if (names[i].value == value)
		{
			return names[i].name;
		}
	}

	return "";
}

template <typename _T, size_t N>
inline constexpr _T _find_enum_value(const NamedEnum<_T>(&names)[N], const string_char *name, _T default_value = _T(0)) {
	for (size_t i = 1; i < N; ++i)
	{
		if (string_tools::equal_insensitive(names[i].name, name))
		{
			return names[i].value;
		}
	}

	return default_value;
}

const string_char *BuildConfiguration::get_enum_name(OptimizationType opt_type) {
	return _find_enum_name(OptimizationTypeNames, opt_type);
}

const string_char *BuildConfiguration::get_enum_name(OptimizationDegree opt_degree) {
	return _find_enum_name(OptimizationDegreeNames, opt_degree);
}

const string_char *BuildConfiguration::get_enum_name(WarningLevel wrn_lvl) {
	return _find_enum_name(WarningLevelNames, wrn_lvl);
}

const string_char *BuildConfiguration::get_enum_name(StandardType standard) {
	return _find_enum_name(StandardTypeNames, standard);
}

const string_char *BuildConfiguration::get_enum_name(SIMDType simd) {
	return _find_enum_name(SIMDTypeNames, simd);
}

OptimizationType BuildConfiguration::get_optimization_type(const string &name) {
	return _find_enum_value(OptimizationTypeNames, name.c_str());
}

OptimizationDegree BuildConfiguration::get_optimization_degree(const string &name) {
	return _find_enum_value(OptimizationDegreeNames, name.c_str());
}

WarningLevel BuildConfiguration::get_warning_level(const string &name) {
	return _find_enum_value(WarningLevelNames, name.c_str());
}

StandardType BuildConfiguration::get_standard_type(const string &name) {
	return _find_enum_value(StandardTypeNames, name.c_str());
}

SIMDType BuildConfiguration::get_simd_type(const string &name) {
	return _find_enum_value(SIMDTypeNames, name.c_str());
}

const string_char *BuildConfiguration::get_compiler_name(CompilerType type, SourceFileType file_type) {
	if (type == CompilerType::GCC)
	{
		switch (file_type)
		{
		case SourceFileType::CPP:
			return "g++";
		default:
			return "gcc";
		}
	}

	return nullptr;
}

#pragma endregion

constexpr StandardType transform_standards(StandardType original, SourceFileType source_type) {
	typedef StandardType E;
	typedef SourceFileType S;

	if (source_type == SourceFileType::None)
	{
		return original;
	}

	switch (original)
	{
		// c (c99 will go to default)

	case E::C11:
		{
			if (source_type == S::CPP)
			{
				return E::Cpp11;
			}

			return E::C11;
		}

	case E::C14:
		{
			if (source_type == S::CPP)
			{
				return E::Cpp14;
			}

			return E::C14;
		}

	case E::C17:
		{
			if (source_type == S::CPP)
			{
				return E::Cpp17;
			}

			return E::C17;
		}

		// case E::C20:
		// {
		// 	if (source_type == S::CPP)
		// 	{
		// 		return E::Cpp20;
		// 	}

		// 	return E::C20;
		// }

	case E::C23:
		{
			if (source_type == S::CPP)
			{
				return E::Cpp23;
			}

			return E::C23;
		}

		// c++

	case E::Cpp11:
		{
			if (source_type == S::C)
			{
				return E::C11;
			}

			return E::Cpp11;
		}

	case E::Cpp14:
		{
			if (source_type == S::C)
			{
				return E::C14;
			}

			return E::Cpp14;
		}

	case E::Cpp17:
		{
			if (source_type == S::C)
			{
				return E::C17;
			}

			return E::Cpp17;
		}

	case E::Cpp20:
		{
			// if (source_type == S::C)
			// {
			// 	return E::C20;
			// }

			return E::Cpp20;
		}

	case E::Cpp23:
		{
			if (source_type == S::C)
			{
				return E::C23;
			}

			return E::Cpp23;
		}

	default:
		return original;
	}
}


