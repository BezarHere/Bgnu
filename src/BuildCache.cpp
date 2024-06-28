#include "BuildCache.hpp"

typedef pair<hash_t *, const string_char *> hash_pointer_info;
typedef pair<int64_t *, const string_char *> int64_pointer_info;

#define CTOR_HASH_PTR_DEF(name) { &cache.name, #name }
#define CTOR_INT64_PTR_DEF_RECORD(name) { &record.name, #name }

static inline ErrorReport load_file_record_table(BuildCache::file_record_table &records,
																								 const FieldDataReader &data);

static inline ErrorReport load_file_record(BuildCache::FileRecord &record, const FieldDataReader &data);

inline BuildCache BuildCache::load(const FieldDataReader &data, ErrorReport &error) {


	BuildCache cache;

	const hash_pointer_info hash_ptrs[] = {
		CTOR_HASH_PTR_DEF(build_hash),
		CTOR_HASH_PTR_DEF(config_hash),
		CTOR_HASH_PTR_DEF(library_hash),
		CTOR_HASH_PTR_DEF(include_dir_hash)
	};

	for (size_t i = 0; i < std::size(hash_ptrs); i++)
	{
		const hash_pointer_info hash_ptr_info = hash_ptrs[i];

		const FieldVar &var = data.try_get_value<FieldVarType::Integer>(
			hash_ptr_info.second, FieldVar(FieldVar::Int(0))
		);

		*hash_ptr_info.first = var.get_int();
	}

	const FieldVar records_data = data.try_get_array<FieldVarType::Dict>("file_records");

	if (records_data.is_null())
	{
		error.code = Error::NoData;
		error.message = "no file records data";
		return cache;
	}


	error = load_file_record_table(cache.file_records, data.branch_reader("file_records"));

	return cache;
}

inline FieldVar::Dict BuildCache::write() const {
	FieldVar::Dict dict{};

	dict["build_hash"] = FieldVar::Int(this->build_hash);
	dict["config_hash"] = FieldVar::Int(this->config_hash);
	dict["library_hash"] = FieldVar::Int(this->library_hash);
	dict["include_dir_hash"] = FieldVar::Int(this->include_dir_hash);


	FieldVar::Dict records{};

	for (const auto &[path, record] : this->file_records)
	{
		FieldVar::Dict record_dict{};

		record_dict.insert_or_assign("source_file", record.source_file.c_str());
		record_dict.insert_or_assign("build_time", FieldVar::Int(record.build_time));
		record_dict.insert_or_assign("last_source_write_time", FieldVar::Int(record.last_source_write_time));
		record_dict.insert_or_assign("hash", FieldVar::Int(record.hash));

		records.insert_or_assign(path.c_str(), record_dict);
	}

	dict["file_records"] = FieldVar{records};

	return dict;
}

inline ErrorReport load_file_record_table(BuildCache::file_record_table &records, const FieldDataReader &data) {

	for (const auto &[key, value] : data.get_data())
	{
		if (!value.is_convertible_to(FieldVarType::Dict))
		{
			ErrorReport report;
			report.code = Error::InvalidType;
			report.message = format_join(
				"file record named \"",
				key,
				"\" should be of type 'dict', but it's of type ",
				value.get_type_name()
			);
			return report;
		}

		BuildCache::FileRecord record;
		ErrorReport report = load_file_record(record, data.branch_reader(key));
		if (report.code != Error::Ok)
		{
			report.message = format_join("in file record \"", key, "\": ", report.message);
			return report;
		}

		records.insert_or_assign(key, record);
	}

	return ErrorReport();
}

inline ErrorReport load_file_record(BuildCache::FileRecord &record, const FieldDataReader &data) {
	const FieldVar &source_file = data.try_get_value<FieldVarType::String>("source_file");

	if (source_file.is_null())
	{
		ErrorReport report;
		report.code = Error::InvalidType;
		report.message = format_join(
			"record's source file should be of type 'string'"
		);
		return report;
	}

	record.source_file = source_file.get_string();

	const int64_pointer_info i64_ptr_info[]{
		CTOR_INT64_PTR_DEF_RECORD(build_time),
		CTOR_INT64_PTR_DEF_RECORD(last_source_write_time),
		{reinterpret_cast<int64_t *>(&record.hash), "hash"}
	};

	for (size_t i = 0; i < std::size(i64_ptr_info); i++)
	{
		const FieldVar &var = data.try_get_value<FieldVarType::Integer>(
			i64_ptr_info[i].second,
			FieldVar(FieldVar::Int())
		);

		*i64_ptr_info[i].first = var.get_int();
	}


	return ErrorReport();
}
