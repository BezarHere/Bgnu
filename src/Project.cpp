#include "Project.hpp"
#include "FieldDataReader.hpp"


Project Project::from_data(const FieldVar::Dict &data, ErrorReport &result) {
	Project project{};

	FieldDataReader reader{"Project", data};

	result = load_various(project, reader);
	if (result)
	{
		Logger::error(result);
		return project;
	}

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

ErrorReport Project::load_various(Project &project, FieldDataReader &reader) {

	const FieldVar::Array &src_slc = \
		reader.try_get_array<FieldVarType::String>("source_selectors").get_array();

	if (src_slc.empty())
	{
		Logger::verbose("found no specified source selectors, using default...");
	}
	else
	{
		Logger::verbose("found %llu source selectors", src_slc.size());
		project.m_source_selectors.clear();
	}


	for (const auto &var : src_slc)
	{
		project.m_source_selectors.emplace_back(var.get_string());
	}


	const FieldVar &output_dir = reader.try_get_value<FieldVarType::String>("output_dir");
	if (output_dir.is_null())
	{
		Logger::verbose(
			"no specified output directory, using default: '%s'",
			project.m_output.dir.c_str()
		);
	}
	else
	{
		project.m_output.dir = output_dir.get_string();
	}

	const FieldVar &output_cache_dir = reader.try_get_value<FieldVarType::String>("output_cache_dir");
	if (output_cache_dir.is_null())
	{
		Logger::verbose(
			"no specified cache directory, using default: '%s'",
			project.m_output.cache_dir.c_str()
		);
	}
	else
	{
		project.m_output.cache_dir = output_cache_dir.get_string();
	}

	const FieldVar &output_name = reader.try_get_value<FieldVarType::String>("output_name");
	if (output_name.is_null())
	{
		Logger::verbose(
			"no specified output name, using default: '%s'",
			project.m_output.name.c_str()
		);
	}
	else
	{
		project.m_output.name = output_name.get_string();
	}

	return ErrorReport();
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

hash_t Project::hash_own(HashDigester &digester) const {
	digester += m_output;

	for (const auto &glob : m_source_selectors)
	{
		digester.add(StrBlob{glob.get_source().c_str(), glob.get_source().length()});
	}

	return digester.value;
}

hash_t Project::hash_cfgs(HashDigester &digester) const {

	for (const auto &[name, cfg] : m_build_configurations)
	{
		digester.add(StrBlob{name.c_str(), name.length()});
		digester += cfg;
	}

	return digester.value;
}

hash_t Project::hash() const {
	HashDigester digester = {};

	hash_own(digester);
	hash_cfgs(digester);

	return digester.value;
}



Error ProjectOutputData::ensure_available() const {
	this->name.parent().create_directory();
	this->cache_dir.create_directory();
	this->dir.create_directory();
	return Error::Ok;
}

FilePath ProjectOutputData::get_result_path() const {
	return FilePath(this->name).resolve(this->dir.resolved_copy());
}
