#include "BuildTools.hpp"



void build_tools::DeleteBuildCache(const Project &project) {
	project.get_output().cache_dir.remove_recursive();
}

void build_tools::DeleteBuildDir(const Project &project) {
	DeleteBuildCache(project);
	project.get_output().dir.remove_recursive();
}

void build_tools::SetupHashes(BuildCache &cache, const Project &proj, const BuildConfiguration *config) {
	cache.build_hash = proj.hash();
	cache.config_hash = config->hash();
}

void build_tools::DeleteUnusedObjFiles(const std::set<FilePath> &object_files, const std::set<FilePath> &used_files) {
	for (const auto &obj : object_files)
	{
		if (used_files.contains(obj))
		{
			continue;
		}

		if (obj.is_file())
		{
			obj.remove();
		}
	}
}
