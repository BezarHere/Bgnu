#include "BuildTools.hpp"



void build_tools::DeleteBuild(const Project &project) {
	project.get_output().cache_dir.remove_recursive();
}

void build_tools::SetupHashes(BuildCache &cache, const Project &proj, const BuildConfiguration *config) {
	cache.build_hash = proj.hash();
	cache.config_hash = config->hash();

	{
		HashDigester digest = {};
		
		for (const auto &lib : config->library_names)
		{
			digest += lib;
		}
		
		for (const auto &lib_dir : config->library_directories)
		{
			digest += lib_dir;
		}

		cache.library_hash = digest.value;
	}

	{
		HashDigester digest = {};
		
		for (const auto &inc : config->include_directories)
		{
			digest += inc;
		}

		cache.library_hash = digest.value;
	}

}
