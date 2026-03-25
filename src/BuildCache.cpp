#include "BuildCache.hpp"

#include <set>

#include "Settings.hpp"

typedef pair<hash_t *, const string_char *> hash_pointer_info;
typedef pair<int64_t *, const string_char *> int64_pointer_info;

#define CTOR_HASH_PTR_DEF(name) \
  { &cache.name, #name }
#define CTOR_INT64_PTR_DEF_RECORD(name) \
  { &record.name, #name }

static inline std::string ParseToHex(hash_t hash);

static inline ErrorReport load_file_record_table(
    BuildCache::file_record_table &records,
    const FieldDataReader &data);

static inline ErrorReport load_file_record(BuildCache::FileRecord &record,
                                           const FieldDataReader &data);

void BuildCache::fix_file_records() {}

std::set<FilePath> BuildCache::extract_compiled_paths() {
  std::set<FilePath> filepaths = {};
  for (const auto &[_, record] : this->file_records)
  {
    filepaths.emplace(record.output_path);
  }
  return filepaths;
}

bool BuildCache::too_out_dated_with(const BuildCache &cache) const {
  const auto expiration_age_seconds =
      Settings::Get("build_expiration_age_sec", DefaultOutdatedCacheTimeSec)
          .get_int();
  const auto last_build_age =
      std::chrono::microseconds(cache.build_time - this->build_time);
  const auto last_build_age_sec_ticks =
      std::chrono::duration_cast<std::chrono::seconds>(last_build_age).count();

  return last_build_age_sec_ticks >= expiration_age_seconds;
}

void BuildCache::override_old_source_record(const FilePath &source_path,
                                            const FileRecord &new_record) {
  this->file_records.insert_or_assign(source_path, new_record);
}

bool BuildCache::is_compatible_with(const BuildCache &older_cache) const {
  return this->build_hash == older_cache.build_hash &&
         this->config_hash == older_cache.config_hash;
}

BuildCache BuildCache::load(const FieldDataReader &data, ErrorReport &error) {

  BuildCache cache;

  const hash_pointer_info hash_ptrs[] = {
    CTOR_HASH_PTR_DEF(build_hash),
    CTOR_HASH_PTR_DEF(config_hash),
  };

  for (size_t i = 0; i < std::size(hash_ptrs); i++)
  {
    const hash_pointer_info hash_ptr_info = hash_ptrs[i];

    const FieldVar &var =
        data.try_get_value<FieldVarType::Integer>(hash_ptr_info.second,
                                                  FieldVar(FieldVar::Int(0)));

    *hash_ptr_info.first = var.get_int();
  }

  cache.build_time =
      data.try_get_value<FieldVarType::Integer>("build_time",
                                                FieldVar(FieldVar::Int(0)))
          .get_int();

  const FieldVar records_data =
      data.try_get_value<FieldVarType::Dict>("file_records");

  if (records_data.is_null())
  {
    error.code = Error::NoData;
    error.message = "no file records data";
    return cache;
  }

  error = load_file_record_table(cache.file_records,
                                 data.branch_reader("file_records"));

  return cache;
}

FieldVar::Dict BuildCache::write() const {
  FieldVar::Dict dict{};

  dict["build_hash"] = FieldVar::Int(this->build_hash);
  dict["config_hash"] = FieldVar::Int(this->config_hash);
  dict["build_time"] = FieldVar::Int(this->build_time);

  FieldVar::Dict records{};

  for (const auto &[path, record] : this->file_records)
  {
    FieldVar::Dict record_dict{};

    record_dict.emplace("output_path", record.output_path);
    record_dict.emplace("source_write_time",
                        FieldVar::Int(record.source_write_time));
    record_dict.emplace("hash", FieldVar::Int(record.hash));
    record_dict.emplace("obj_hash", FieldVar::Int(record.obj_hash));
    record_dict.emplace("obj_size", FieldVar::Int(record.obj_size));

    records.insert_or_assign(path.c_str(), FieldVar(record_dict));
  }

  dict["file_records"] = FieldVar{ records };

  return dict;
}

inline std::string ParseToHex(hash_t hash) {
  char buffer[32] = { 0 };
  snprintf(buffer, std::size(buffer), "%lX", hash);
  return { buffer };
}

inline ErrorReport load_file_record_table(
    BuildCache::file_record_table &records,
    const FieldDataReader &data) {

  for (const auto &[key, value] : data.get_data())
  {
    if (!value.is_convertible_to(FieldVarType::Dict))
    {
      ErrorReport report;
      report.code = Error::InvalidType;
      report.message =
          format_join("file record named \"",
                      key,
                      "\" should be of type 'dict', but it's of type ",
                      value.get_type_name());
      return report;
    }

    BuildCache::FileRecord record;
    ErrorReport report = load_file_record(record, data.branch_reader(key));
    if (report.code != Error::Ok)
    {
      report.message =
          format_join("in file record \"", key, "\": ", report.message);
      return report;
    }

    records.insert_or_assign(key, record);
  }

  return ErrorReport();
}

inline ErrorReport load_file_record(BuildCache::FileRecord &record,
                                    const FieldDataReader &data) {
  const FieldVar &output_path =
      data.try_get_value<FieldVarType::String>("output_path");

  if (output_path.is_null())
  {
    ErrorReport report;
    report.code = Error::InvalidType;
    report.message =
        format_join("record's output path should be of type 'string'");
    return report;
  }

  record.output_path = output_path.get_string();

  const int64_pointer_info i64_ptr_info[]{
    CTOR_INT64_PTR_DEF_RECORD(source_write_time),
  };

  for (size_t i = 0; i < std::size(i64_ptr_info); i++)
  {
    const FieldVar &var =
        data.try_get_value<FieldVarType::Integer>(i64_ptr_info[i].second,
                                                  FieldVar(FieldVar::Int()));

    *i64_ptr_info[i].first = var.get_int();
  }

  record.hash =
      data.try_get_value<FieldVarType::Integer>("hash", FieldVar::Int())
          .get_int();

  record.obj_size =
      data.try_get_value<FieldVarType::Integer>("obj_size", FieldVar::Int())
          .get_int();

  record.obj_hash =
      data.try_get_value<FieldVarType::Integer>("obj_hash", FieldVar::Int())
          .get_int();

  return ErrorReport();
}