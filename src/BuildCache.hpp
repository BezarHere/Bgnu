#pragma once
#include <set>

#include "FieldDataReader.hpp"
#include "FilePath.hpp"
#include "HashTools.hpp"
#include "base.hpp"
#include "misc/Time.hpp"

struct BuildCache
{
  // 3 days
  static constexpr t::microsecond_t DefaultOutdatedCacheTimeSec = 60 * 60 * 24 * 3;

  struct FileRecord
  {
    FilePath output_path = {};
    hash_t hash = 0;
    t::microsecond_t source_write_time = 0;
  };
  typedef std::map<FilePath, FileRecord> file_record_table;

  // removes old duplicates (records with the same source path)
  void fix_file_records();

  std::set<FilePath> extract_compiled_paths();

  bool too_out_dated_with(const BuildCache &cache) const;

  // removes the last record with the same source file
  void override_old_source_record(const FilePath &source_path, const FileRecord &new_record);

  bool is_compatible_with(const BuildCache &older_cache) const;

  static BuildCache load(const FieldDataReader &data, ErrorReport &error);
  FieldVar::Dict write() const;

  t::microsecond_t build_time;
  hash_t build_hash = 0;
  hash_t config_hash = 0;

  // source file (key), a record (value)
  file_record_table file_records;
};
