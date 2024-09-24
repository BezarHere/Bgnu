#pragma once
#include "BuildConfiguration.hpp"
#include <map>

enum class BuildOutputType
{
	None,
	Executable,
	StaticLibrary,
	DynamicLibrary,
};

struct ProjectOutputData
{
	// creates folders and stuff
	Error ensure_available() const;

	// returns the final path of the binary/library
	FilePath get_result_path() const;

	inline hash_t hash() const {
		return HashTools::combine(
			(hash_t)type,
			HashTools::hash(name.get_text()),
			HashTools::hash(dir.get_text()),
			HashTools::hash(cache_dir.get_text())
		);
	}

	BuildOutputType type = BuildOutputType::Executable;
	FilePath name = "out/main";
	FilePath dir = "out";
	FilePath cache_dir = "out/cache/";
};

struct ProjectModifier
{
	enum class ModifierType : uint8_t
	{
		Set,
		Del,

		Add,
		Sub,
		Mul,
		Div,
	};

	ModifierType type;
	Glob config_glob; // TODO: extended glob; separating to different globs ("GL*B1|GL*B2")
	string name;
	FieldVar var;
};

class Project
{
public:
	typedef std::map<FieldVar::String, BuildConfiguration> BuildConfigMap;

	static Project from_data(const FieldVar::Dict &data, ErrorReport &result);

	inline const ProjectOutputData &get_output() const { return m_output; }
	inline const vector<Glob> &get_source_selector() const { return m_source_selectors; }
	inline const BuildConfigMap &get_build_configs() const { return m_build_configurations; }

	vector<FilePath::iterator_entry> get_available_files() const;
	vector<FilePath> get_source_files() const;

	bool is_matching_source(const StrBlob &path) const;

	hash_t hash_own(HashDigester &digester) const;
	hash_t hash_cfgs(HashDigester &digester) const;
	hash_t hash() const;

	FilePath source_dir = FilePath::get_working_directory();
private:
	static ErrorReport load_various(Project &project, FieldDataReader &reader);

private:
	ProjectOutputData m_output = {};
	vector<Glob> m_source_selectors = {"**/*.c", "**/*.cpp", "**/*.cc", "**/*.cxx"};

	vector<ProjectModifier> m_modifiers;

	BuildConfigMap m_build_configurations;
};
