#pragma once
#include "BuildConfiguration.hpp"
#include <map>

struct ProjectOutputData
{
	FilePath path = "out/main";
	FilePath cache_dir = "out/cache/";
};

class Project
{
public:
	static Project from_data(const FieldVar::Dict &data, ErrorReport &result);

private:
	ProjectOutputData m_output = {};
	vector<Glob> m_source_selectors = {"**/*.c", "**/*.cpp", "**/*.cc", "**/*.cxx"};

	std::map<FieldVar::String, BuildConfiguration> m_build_configurations;
};
