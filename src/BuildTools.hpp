#pragma once
#include "Project.hpp"
#include "BuildCache.hpp"

#include <set>

namespace build_tools
{

	extern void DeleteBuildCache(const Project &project);
	extern void DeleteBuildDir(const Project &project);
	extern void SetupHashes(BuildCache &cache, const Project &proj, const BuildConfiguration *config);

	extern void DeleteUnusedObjFiles(const std::set<FilePath> &object_files, const std::set<FilePath> &used_files);

}

