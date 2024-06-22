#pragma once
#include "SourceTools.hpp"


class SourceProcessor
{
public:
	typedef string dependency_name;
	struct DependencyInfo
	{
		SourceFileType type;
		hash_t hash = 0;

		FilePath intermediate_output;

		vector<dependency_name> sub_dependencies;
		vector<FilePath> included_directories;
	};

	typedef std::map<string, vector<dependency_name>> dependency_map;
	typedef std::map<FilePath, DependencyInfo> dependency_info_map;
	typedef std::map<FilePath, hash_t> rainbow_table;

	enum Flags : uint8_t
	{
		eFlag_None = 0,
		eFlag_WarnAbsentDependencies = 0x01,
	};


	void process();

	void add_file(const FilePath &filepath);

	hash_t get_file_hash(const FilePath &filepath) const;

	const dependency_map &gen_dependency_map() const;

	inline Flags get_flags() const { return m_flags; }
	inline bool has_flags(Flags flags) const { return ((int)m_flags & (int)flags) != 0; }
	inline void add_flags(Flags flags) { m_flags = Flags((int)m_flags | (int)flags); }
	inline void remove_flags(Flags flags) { m_flags = Flags((int)m_flags & ~(int)flags); }
	inline void clear_flags() { m_flags = eFlag_None; }

private:
	bool _is_processing_path(const FilePath &filepath) const;
	void _push_processing_path(const FilePath &filepath);
	void _pop_processing_path(const FilePath &filepath);
	void _process_dependencies(const FilePath &filepath);

	static FilePath _find_dependency(const size_t name_index,
																	 const FilePath &file,
																	 const DependencyInfo &info);

	static bool _dependency_exists(const FilePath &path, SourceFileType type);

private:
	SourceFileType m_type;
	Flags m_flags;
	vector<FilePath> m_inputs;
	vector<FilePath> m_loading_stack;
	dependency_info_map m_info_map;
};
