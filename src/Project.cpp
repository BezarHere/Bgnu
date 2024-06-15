#include "Project.hpp"
#include "FieldDataReader.hpp"


Project Project::from_data(const FieldVar::Dict &data, ErrorReport &result) {
	Project project{};

	FieldDataReader reader{"Project", data};


	const FieldVar &build_configs = reader.try_get_value<FieldVarType::Dict>("build_configurations");

	if (build_configs.is_null())
	{
		result.code = Error::NoData;
		result.message = "No build configs ('build_configurations') set";
		Logger::error("Project: %s", to_cstr(result.message));
		return project;
	}

	for (const auto &[key, value] : build_configs.get_dict())
	{
		Logger::debug("Started parsing build config: %s", to_cstr(key));
		if (value.get_type() != FieldVarType::Dict)
		{
			Logger::error(
				"%s: Invalid value for build configuration '%s': expected type %s, gotten type %s",
				to_cstr(reader.get_context()),
				to_cstr(key),
				to_cstr(FieldVar::get_name_for_type(FieldVarType::Dict)),
				to_cstr(value.get_type_name())
			);

			result.code = Error::InvalidType;
			result.message = format_join("ill-typed build configuration \"", key, "\"");
			return project;
		}


		BuildConfiguration config = BuildConfiguration::from_data(
			FieldDataReader(
				format_join(reader.get_context(), "::", "BuildCfg[", key, ']'),
				value.get_dict()
			), result
		);

		if (result.code != Error::Ok)
		{
			Logger::debug("Failure in parsing and loading build cfg named '%s'", to_cstr(key));
			return project;
		}

		project.m_build_configurations.insert_or_assign(key, config);
	}

	return project;
}

vector<FilePath::iterator_entry> Project::get_available_files() const {
	vector<FilePath::iterator_entry> entries{};
	vector<FilePath::iterator> iterators{};

	entries.reserve(1024);

	iterators.push_back(this->source_dir.create_iterator());

	while (!iterators.empty())
	{
		const auto current_iter = iterators.back();
		iterators.pop_back();

		for (const auto &path : current_iter)
		{
			if (path.is_directory())
			{
				iterators.emplace_back(path);
				continue;
			}

			if (path.is_regular_file())
			{
				entries.push_back(path);
				continue;
			}

			// skips other non-regular files
		}
	}

	return entries;
}

vector<FilePath> Project::get_source_files() const {
	vector<FilePath> result_paths{};

	for (const auto &p : get_available_files())
	{
		FilePath path = p;
		

		if (is_matching_source(path.get_text()))
		{
			result_paths.push_back(path);
		}

	}

	return result_paths;
}

bool Project::is_matching_source(const StrBlob &path) const {
	for (const Glob &glob : m_source_selectors)
	{
		if (glob.test(path))
		{
			return true;
		}
	}
	return false;
}

Error ProjectOutputData::ensure_available() const {
	this->path.parent().create_directory();
	this->cache_dir.create_directory();
	return Error::Ok;
}
