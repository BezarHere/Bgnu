#include "BuildCommand.hpp"

const string RebuildArgs[] = {"-r", "--rebuild"};
const string ResaveArgs[] = {"--resave"};

constexpr const char *InputFilePrefix = "-i=";

Error commands::BuildCommand::execute(ArgumentReader &reader) {

	bool rebuild = Argument::try_use(
		reader.extract_any(Blob<const string>(RebuildArgs))
	);

	bool resave = Argument::try_use(
		reader.extract_any(Blob<const string>(ResaveArgs))
	);

	Logger::verbose("rebuild = %s", to_boolalpha(rebuild));
	Logger::verbose("resave = %s", to_boolalpha(resave));

	const string &input_file_str = Argument::try_get_value(
		reader.extract_matching(
			[](const Argument &arg) {
				return arg.get_value().starts_with(InputFilePrefix);
			}
		)
	);

	Logger::verbose("input file string = \"%s\"", to_cstr(input_file_str));

	const FilePath input_file_path = \
		input_file_str.empty() ? _default_filepath() : FilePath(input_file_str);

	Logger::verbose("input path = \"%s\"", to_cstr(input_file_path.get_text()));

	if (!input_file_path.exists())
	{
		Logger::error("No project file exists at \"%s\"", to_cstr(input_file_path.get_text()));

		return Error::Failure;
	}

	if (!input_file_path.is_file())
	{
		Logger::error(
			"The path \"%s\" is to a directory, not a project file",
			to_cstr(input_file_path.get_text())
		);

		return Error::Failure;
	}

	return Error::Ok;
}

FilePath commands::BuildCommand::_default_filepath() {
	return FilePath::get_working_directory().join_path(".bgnu");
}
