#pragma once
#include "SourceTools.hpp"
#include "BuildCache.hpp"


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

	struct InputFilePath
	{
		FilePath path;
		SourceFileType source_type;
	};

	typedef std::map<string, vector<dependency_name>> dependency_map;
	typedef std::map<FilePath, DependencyInfo> dependency_info_map;
	typedef std::map<FilePath, hash_t> rainbow_table;
	typedef BuildCache::file_record_table file_record_table;
	typedef vector<pair<FilePath, hash_t>> file_change_list;

	enum Flags : uint8_t
	{
		eFlag_None = 0,
		eFlag_WarnAbsentDependencies = 0x01,
	};


	void process();

	void add_file(const InputFilePath &input);
	inline dependency_info_map &get_dependency_info_map() { return m_info_map; }
	inline const dependency_info_map &get_dependency_info_map() const { return m_info_map; }

	hash_t get_file_hash(const FilePath &filepath) const;
	bool has_file_hash(const FilePath &filepath) const;

	file_change_list gen_file_change_table(bool inputs_only = true) const;

	dependency_map gen_dependency_map() const;

	inline Flags get_flags() const { return m_flags; }
	inline bool has_flags(Flags flags) const { return ((int)m_flags & (int)flags) != 0; }
	inline void add_flags(Flags flags) { m_flags = Flags((int)m_flags | (int)flags); }
	inline void remove_flags(Flags flags) { m_flags = Flags((int)m_flags & ~(int)flags); }
	inline void clear_flags() { m_flags = eFlag_None; }

	inline Blob<const InputFilePath> get_inputs() const { return {m_inputs.data(), m_inputs.size()}; }
	inline std::map<FilePath, FilePath> get_file_records_src2out_map() const {
		return m_file_records_src2out_map;
	}

	void set_file_records(const file_record_table &records);

	vector<FilePath> included_directories;
private:

	bool _is_processing_path(const FilePath &filepath) const;
	void _push_processing_path(const FilePath &filepath);
	void _pop_processing_path(const FilePath &filepath);
	void _process_input(const InputFilePath &input);

	void _rebuild_file_record_src2out_map();

	FilePath _find_dependency(const dependency_name &name, const FilePath &file, SourceFileType type) const;

	static bool _dependency_exists(const FilePath &path, SourceFileType type);

private:
	Flags m_flags;
	vector<InputFilePath> m_inputs;
	vector_stack<InputFilePath> m_input_stack;
	vector_stack<FilePath> m_loading_stack;
	dependency_info_map m_info_map;
	file_record_table m_file_records;
	std::map<FilePath, FilePath> m_file_records_src2out_map;
};
