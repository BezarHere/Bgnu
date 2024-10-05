#include "NewCommand.hpp"
#include "BuildCommand.hpp"
#include "Project.hpp"
#include "FieldFile.hpp"

static const std::string OverwriteFlags[] = {"-o", "--overwrite"};


Error commands::NewCommand::execute(ArgumentReader &reader) {
	const FilePath save_file = reader.read_or(BuildCommand::_default_filepath().c_str());
	const bool overwrite = reader.check_flag_any(OverwriteFlags);

	if (save_file.exists() && !overwrite)
	{
		Logger::warning(
			"there is already a file at '%s', please add the flag '%s' or '%s' to overwrite",
			save_file.c_str(),
			OverwriteFlags[0].c_str(),
			OverwriteFlags[1].c_str()
		);
		return Error::Ok;
	}

	Project project = Project::GetDefault();
	FieldVar var = {};
	Project::to_data(project, var);

	if (var.get_type() != FieldVarType::Dict)
	{
		Logger::error(
			"expecting a dict variable from default project serialization, found var of type '%s'",
			FieldVar::get_name_for_type(var.get_type())
		);
	}

	FieldFile::dump(save_file, var.get_dict());
	return Error::Ok;
}

Error commands::NewCommand::get_help(ArgumentReader &reader, string &out) {
	out.append(get_info().desc);
	return Error();
}
