#pragma once
#include "base.hpp"
#include "FilePath.hpp"

enum class SourceFileType
{
	C,
	CPP = C,

};

class SourceTools
{
public:
	static void get_dependencies(const StrBlob &file, SourceFileType type, vector<string> &out);
};

class SourceProcessor
{
public:
	struct DependencyInfo
	{
		hash_t hash = 0;
		vector<FilePath> sub_dependencies;
		vector<FilePath> included_directories;
	};

	typedef std::map<string, vector<string>> dependency_map;
	typedef std::map<FilePath, DependencyInfo> dependency_info_map;

	void add_file(const FilePath &filepath);
	void add_file(const StrBlob &source);

	void process();

	hash_t get_file_hash(const FilePath &filepath) const;

private:
	bool is_processing_path(const FilePath &filepath) const;
	void process_dependencies(const FilePath &filepath);

private:
	SourceFileType m_type;
	dependency_info_map m_info_map;
	vector<FilePath> m_in_loading;
};
