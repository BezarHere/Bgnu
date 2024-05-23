#pragma once
#include "base.hpp"
#include "FieldVar.hpp"

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

	Latest = 0xFF
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

class BuildConfiguration
{
public:


private:
	vector<string> m_preprocessor_args;
	vector<string> m_linker_args;
	vector<string> m_assembler_args;

	vector<string> m_library_names;

	vector<FilePath> m_library_directories;
	vector<FilePath> m_include_directories;

	vector<string> m_predefines;

	OptimizationInfo m_optimization = {};
	WarningReportInfo m_warnings = {};

	bool m_fatal_errors = true;
	bool print_stats = false;
	bool dynamically_linkable = true;

	StandardType m_standard = StandardType::Cpp17;
	SIMDType m_simd_type = SIMDType::AVX2;
};
