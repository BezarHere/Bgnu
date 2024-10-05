#pragma once
#include "base.hpp"
#include "HashTools.hpp"
#include "FieldDataReader.hpp"
#include "FilePath.hpp"
#include "misc/Error.hpp"
#include "Glob.hpp"
#include "SourceTools.hpp" // for SourceFileType

#include "utility/NField.hpp"

enum class CompilerType
{
	GCC,
	CLANG,
	MSVC,
};

enum class OptimizationType : uint8_t
{
	None = 0,

	Release,
	Debug,

	Size,
	SpeedUnreliable,
};

enum class OptimizationDegree : uint8_t
{
	None,

	Low,
	Medium,
	High,

	Extreme,
};

enum class WarningLevel : uint8_t
{
	None = 0,
	All,
	Extra
};

enum class StandardType : uint8_t
{
	None = 0,

	C99,
	C11,
	C14,
	C17,
	C23,

	Cpp11,
	Cpp14,
	Cpp17,
	Cpp20,
	Cpp23,

	Latest
};

enum class SIMDType : uint8_t
{
	None = 0,

	SSE,
	SSE2,
	SSE3,
	SSE3_1,
	SSE4,

	AVX,
	AVX2,
	AVX512,
};

struct OptimizationInfo
{
	inline hash_t hash() const noexcept {
		return HashTools::combine((int)type.field(), (int)degree.field());
	}

	NField<OptimizationType> type = {"type", OptimizationType::Release};
	NField<OptimizationDegree> degree = {"degree", OptimizationDegree::High};
};

struct WarningReportInfo
{
	inline hash_t hash() const noexcept {
		return HashTools::combine((int)level.field(), (int)pedantic.field());
	}

	NField<WarningLevel> level = {"level", WarningLevel::All};
	NField<bool> pedantic = {"pedantic", false};
};

enum class BuildConfigurationDefaultType
{
	Debug,
	Release
};

struct BuildConfiguration
{
	inline BuildConfiguration() {}

	void _put_predefines(vector<string> &output) const;
	void _put_flags(vector<string> &output) const;

	void _put_standards(vector<string> &output, SourceFileType type) const;
	void _put_optimization(vector<string> &output) const;
	void _put_warnings(vector<string> &output) const;

	void _put_misc(vector<string> &output) const;

	void _put_includes(vector<string> &output) const;
	void _put_libraries(vector<string> &output) const;

	void _put_sub_args(vector<string> &output) const;

	void build_arguments(vector<string> &output, const StrBlob &input_file,
											 const StrBlob &output_file, SourceFileType type) const;

	void build_link_arguments(vector<string> &output,
														const Blob<const StrBlob> &files, const StrBlob &ouput_file) const;

	hash_t hash() const;

	static BuildConfiguration GetDefault(BuildConfigurationDefaultType default_mode);
	static BuildConfiguration from_data(FieldDataReader reader, ErrorReport &report);
	static FieldVar::Dict to_data(const BuildConfiguration &config, ErrorReport &report);

	static const char *get_enum_name(OptimizationType opt_type);
	static const char *get_enum_name(OptimizationDegree opt_degree);
	static const char *get_enum_name(WarningLevel wrn_lvl);
	static const char *get_enum_name(StandardType standard);
	static const char *get_enum_name(SIMDType simd);

	static OptimizationType get_optimization_type(const string &name);
	static OptimizationDegree get_optimization_degree(const string &name);
	static WarningLevel get_warning_level(const string &name);
	static StandardType get_standard_type(const string &name);
	static SIMDType get_simd_type(const string &name);

	static const string_char *get_compiler_name(CompilerType type, SourceFileType file_type);

	// values should be either a string OR a null 
	NField<FieldVar::Dict> predefines = {"predefines"};

	NField<OptimizationInfo> optimization = {"optimization", OptimizationInfo{}};
	NField<WarningReportInfo> warnings = {"warnings", WarningReportInfo{}};

	NField<CompilerType> compiler_type = {"compiler_type", CompilerType::GCC};
	NField<StandardType> standard = {"standard", StandardType::Cpp20};
	NField<SIMDType> simd_type = {"simd_type", SIMDType::AVX2};

	NField<bool> exit_on_errors = {"exit_on_errors", true};
	NField<bool> print_stats = {"print_stats", false};
	NField<bool> print_includes = {"print_includes", false};
	NField<bool> dynamically_linkable = {"dynamically_linkable", true};

	NField<vector<string>> preprocessor_args = {"preprocessor_args"};
	NField<vector<string>> linker_args = {"linker_args"};
	NField<vector<string>> assembler_args = {"assembler_args"};

	NField<vector<string>> library_names = {"library_names"};

	NField<vector<FilePath>> library_directories = {"library_directories"};
	NField<vector<FilePath>> include_directories = {"include_directories"};
};
