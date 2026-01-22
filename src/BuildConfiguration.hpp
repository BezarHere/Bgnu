#pragma once
#include "base.hpp"
#include "HashTools.hpp"
#include "FieldDataReader.hpp"
#include "FilePath.hpp"
#include "misc/Error.hpp"
#include "Glob.hpp"
#include "code/SourceTools.hpp" // for SourceFileType

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
	C2x,
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

enum class BuildType : uint8_t
{
  Binary,
  Library,
  Intermediates
};

struct OptimizationInfo
{
	inline hash_t hash() const noexcept {
		return HashTools::combine((int)type.field(), (int)degree.field());
	}

	NField<OptimizationType> type = {"type", OptimizationType::Release};
	NField<OptimizationDegree> degree = {"degree", OptimizationDegree::High};
	NField<bool> debug_optimizing = {"debug_optimizing-Og", false};
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

	void _put_compiler(vector<string> &output, SourceFileType source_type) const;

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
														const Blob<const StrBlob> &files,
														const StrBlob &ouput_file, SourceFileType type) const;

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

	NSerializable<BuildType> build_type = {"type", BuildType::Binary, NSerializationStance::Optional};

	// values should be either a string OR a null 
	NSerializable<FieldVar::Dict> predefines = {"predefines"};

	NSerializable<OptimizationInfo> optimization = {"optimization", OptimizationInfo{}};
	NSerializable<WarningReportInfo> warnings = {"warnings", WarningReportInfo{}};

	NSerializable<CompilerType> compiler_type = {"compiler_type", CompilerType::GCC};
	NSerializable<StandardType> standard = {"standard", StandardType::Cpp20};
	NSerializable<SIMDType> simd_type = {"simd_type", SIMDType::AVX2, NSerializationStance::Optional};

	NSerializable<bool> exit_on_errors = {"exit_on_errors", true};
	NSerializable<bool> print_stats = {"print_stats", false};
	NSerializable<bool> print_includes = {"print_includes", false};
	NSerializable<bool> dynamically_linkable = {"dynamically_linkable", true};
	NSerializable<bool> sanitize_addresses = {"sanitize_addresses", false, NSerializationStance::Optional};
	NSerializable<bool> static_stdlib = {"static_stdlib", false, NSerializationStance::Optional};

	NSerializable<vector<string>> preprocessor_args = {"preprocessor_args"};
	NSerializable<vector<string>> linker_args = {"linker_args"};
	NSerializable<vector<string>> assembler_args = {"assembler_args"};

	NSerializable<vector<string>> library_names = {"library_names"};

	NSerializable<vector<FilePath>> library_directories = {"library_directories"};
	NSerializable<vector<FilePath>> include_directories = {"include_directories"};
};
