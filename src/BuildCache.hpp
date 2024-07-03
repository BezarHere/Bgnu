#pragma once
#include "base.hpp"
#include "FieldDataReader.hpp"
#include "FilePath.hpp"
#include "HashTools.hpp"

struct BuildCache
{
	struct FileRecord
	{
		FilePath source_file;
		hash_t hash = 0;
		int64_t build_time = 0;
		int64_t last_source_write_time = 0;
	};
	typedef std::map<FilePath, FileRecord> file_record_table;

	static BuildCache load(const FieldDataReader &data, ErrorReport &error);
	FieldVar::Dict write() const;

	hash_t build_hash = 0;
	hash_t config_hash = 0;
	hash_t library_hash = 0;
	hash_t include_dir_hash = 0;

	// intermidiate file: it's record
	file_record_table file_records;
};
