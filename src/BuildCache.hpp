#pragma once
#include "base.hpp"
#include "FieldDataReader.hpp"
#include "FilePath.hpp"
#include "HashTools.hpp"

struct BuildCache
{
	struct FileRecord
	{
		FilePath output_path = {};
		hash_t hash = 0;
		int64_t build_time = 0;
		int64_t last_source_write_time = 0;
	};
	typedef std::map<FilePath, FileRecord> file_record_table;

	// removes old duplicates (records with the same source path)
	void fix_file_records();

	// removes the last record with the same source file
	void override_old_source_record(const FilePath &source_path, const FileRecord &new_record);

	static BuildCache load(const FieldDataReader &data, ErrorReport &error);
	FieldVar::Dict write() const;

	hash_t build_hash = 0;
	hash_t config_hash = 0;
	hash_t library_hash = 0;
	hash_t include_dir_hash = 0;

	// source file (key), a record (value)
	file_record_table file_records;
};
