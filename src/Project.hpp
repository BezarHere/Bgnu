#pragma once
#include "BuildConfiguration.hpp"
#include <map>

struct ProjectOutputData
{
	// creates folders and stuff
	Error ensure_available() const;

	FilePath path = "out/main";
	FilePath cache_dir = "out/cache/";
};

class Project
{
public:
	typedef std::map<FieldVar::String, BuildConfiguration> BuildConfigMap;

	static Project from_data(const FieldVar::Dict &data, ErrorReport &result);

	inline const ProjectOutputData &get_output() const { return m_output; }
	inline const vector<Glob> &get_source_selector() const { return m_source_selectors; }
	inline const BuildConfigMap &get_build_configs() const { return m_build_configurations; }

private:
	ProjectOutputData m_output = {};
	vector<Glob> m_source_selectors = {"**/*.c", "**/*.cpp", "**/*.cc", "**/*.cxx"};

	BuildConfigMap m_build_configurations;
};
