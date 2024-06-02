#include "BuildCommand.hpp"

const string RebuildArgs[] = {"-r", "--rebuild"};
const string ResaveArgs[] = {"--resave"};

constexpr const char *InputFilePrefix = "-i=";

Error commands::BuildCommand::execute(ArgumentReader &reader) {

	bool rebuild = reader.extract_any(Blob<const string>(RebuildArgs))->try_use();
	bool resave = reader.extract_any(Blob<const string>(ResaveArgs))->try_use();

	const string &input_file_str = reader.extract_matching(
		[](const Argument &arg) {
			return arg.value.starts_with(InputFilePrefix);
		}
	)->try_get_value();

	return Error::Ok;
}