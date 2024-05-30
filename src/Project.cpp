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
