#pragma once
#include "Project.hpp"
#include "BuildCache.hpp"

namespace build_tools
{

	extern void DeleteBuild(const Project &project);
	extern void SetupHashes(BuildCache &cache, const Project &proj, const BuildConfiguration *config);

}

