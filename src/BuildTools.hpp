#pragma once
#include "Project.hpp"
#include "BuildCache.hpp"

#include <set>

namespace build_tools
{
	enum _ExecuteFlags : uint8_t
	{
		eExcFlag_Printout = 0x01,
		eExcFlag_PrintoutProgress = 0x02,
	};

	typedef std::underlying_type_t<_ExecuteFlags> ExecuteFlags;

	struct ExecuteParameter
	{
		ExecuteFlags flags = eExcFlag_Printout;
		std::ostream *out;
		StaticString<128> name;
		bool critical = false;
		std::vector<std::string> args;
	};

	extern void DeleteBuildCache(const Project &project);
	extern void DeleteBuildDir(const Project &project);
	extern void SetupHashes(BuildCache &cache, const Project &proj, const BuildConfiguration *config);

	extern void DeleteUnusedObjFiles(const std::set<FilePath> &object_files, const std::set<FilePath> &used_files);

	extern std::vector<int> Execute(const Blob<const ExecuteParameter> &params);
	extern std::vector<int> Execute_Multithreaded(const Blob<const ExecuteParameter> &params, uint32_t batches);

	extern errno_t _Execute_Inner(const Blob<const std::string> &args,
																const Blob<const ExecuteParameter> &params,
																Blob<int> results);

	extern errno_t _ExecuteParallel_Inner(const Blob<const std::string> &args,
																				const Blob<const ExecuteParameter> &params,
																				Blob<int> results,
																				uint32_t batches);


	extern SourceFileType GetDominantSourceType(Blob<const SourceFileType> file_types);

	extern SourceFileType DefaultSourceFileTypeForFilePath(const FilePath::char_type *path);
	extern SourceFileType DefaultSourceFileTypeForFilePath(const FilePath &path);

	extern SourceFileType DefaultSourceFileTypeForExtension( FilePath::string_blob extension);
	extern SourceFileType DefaultSourceFileTypeForExtension(const FilePath::string_type &extension);

	extern const char *GetSourceFileTypeName(SourceFileType type);
}

