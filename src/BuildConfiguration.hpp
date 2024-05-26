#pragma once
#include "base.hpp"
#include "FieldVar.hpp"
#include "Error.hpp"

using FilePath = string;
using Glob = string;

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
	Few,
	Most,
	All
};

enum class StandardType : uint8_t
{
	None = 0,

	C99,
	C11,
	C14,
	C17,
	C23,

	Cpp99,
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
	OptimizationType type = OptimizationType::Release;
	OptimizationDegree degree = OptimizationDegree::High;
};

struct WarningReportInfo
{
	WarningLevel level = WarningLevel::Most;
	bool pedantic = false;
};

struct BuildConfiguration
{
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
