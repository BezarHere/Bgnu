#pragma once
#include "BuildConfiguration.hpp"

struct ProjectOutputData
{
	FilePath path = "out/main";
	FilePath cache_dir = "out/cache/";
};

class Project
{
public:
	Project(const FieldVar::Dict &data);

private:
	ProjectOutputData m_output = {};
	vector<Glob> m_source_selectors = {"**/*.c", "**/*.cpp", "**/*.cc", "**/*.cxx"};

	vector<BuildConfiguration> m_build_configurations;
};
