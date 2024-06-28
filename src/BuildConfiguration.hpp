#pragma once
#include "base.hpp"
#include "HashTools.hpp"
#include "FieldDataReader.hpp"
#include "FilePath.hpp"
#include "misc/Error.hpp"
#include "Glob.hpp"
#include "SourceTools.hpp" // for SourceFileType

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
		return HashTools::combine((int)type, (int)degree);
	}

	OptimizationType type = OptimizationType::Release;
	OptimizationDegree degree = OptimizationDegree::High;
};

struct WarningReportInfo
{
	inline hash_t hash() const noexcept {
		return HashTools::combine((int)level, (int)pedantic);
	}

	WarningLevel level = WarningLevel::All;
	bool pedantic = false;
};

struct BuildConfiguration
{
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

	static BuildConfiguration from_data(FieldDataReader reader, ErrorReport &report);

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

	// values should be either a string OR a null 
	FieldVar::Dict predefines;

	OptimizationInfo optimization = {};
	WarningReportInfo warnings = {};

	StandardType standard = StandardType::Cpp17;

	bool exit_on_errors = true;
	bool print_stats = false;
	bool print_includes = false;
	bool dynamically_linkable = true;

	SIMDType simd_type = SIMDType::AVX2;

	vector<string> preprocessor_args;
	vector<string> linker_args;
	vector<string> assembler_args;

	vector<string> library_names;

	vector<FilePath> library_directories;
	vector<FilePath> include_directories;
};
