#include "BuildCommand.hpp"
#include "../FieldFile.hpp"
#include "../SourceTools.hpp"

const string RebuildArgs[] = {"-r", "--rebuild"};
const string ResaveArgs[] = {"--resave"};

constexpr const char *InputFilePrefix = "-i=";

typedef SourceProcessor::dependency_map dependency_map;

static void dump_dep_map(const dependency_map &map, const FilePath &cache_folder);

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

	const FilePath project_dir = input_file_path.parent();
	const FieldVar project_file_data = FieldFile::load(input_file_path);

	// std::cout << '\n' << "project file data: " << FieldFile::write(project_file_data.get_dict()) << '\n';
	FieldFile::dump(string(input_file_path) + string("-out"), project_file_data.get_dict());

	ErrorReport report{};
	Project project = Project::from_data(project_file_data.get_dict(), report);
	project.source_dir = project_dir;

	if (report.code != Error::Ok)
	{
		Logger::error(
			"Building project reported an err code=%d: %s", (int)report.code, to_cstr(report.message)
		);
		return report.code;
	}

	if (project.get_build_configs().empty())
	{
		Logger::error(
			"No build configurations are available"
		);
		return Error::NoConfig;
	}

	project.get_output().ensure_available();


	std::map<string, vector<string>> dependencies{};

	for (const auto &path : project.get_source_files())
	{
		auto stream = path.stream_read(false);

		string output{};

		stream.seekg(0, std::ios::end);
		// TODO: size check, like, loading a 3gb file to memory is quite bad
		output.resize(stream.tellg() + std::streamoff(1));
		stream.seekg(0, std::ios::beg);

		stream.read(output.data(), static_cast<std::streamsize>(output.size()));

		vector<string> deps{};
		SourceTools::get_dependencies({output.data(), output.size()}, SourceType::C, deps);
		dependencies.insert_or_assign(
			string(path),
			deps
		);


		// std::cout << output << '\n';

		// size_t newlines = 0;

		// for (string_char c : output)
		// {
		// 	if (StringTools::is_newline(c))
		// 	{
		// 		newlines++;
		// 	}
		// }

		// std::cout << "newlines " << newlines << ": at " << path << '\n';
	}

	dump_dep_map(dependencies, project.get_output().cache_dir);

	return Error::Ok;
}

FilePath commands::BuildCommand::_default_filepath() {
	return FilePath::get_working_directory().join_path(".bgnu");
}

void dump_dep_map(const dependency_map &map, const FilePath &cache_folder) {
	FilePath output_path = cache_folder.join_path(".deps");
	std::ofstream stream = output_path.stream_write(false);

	// We can do it by converting 'map' to field var and save it
	// sadly, im too lazy rn

	for (const auto &[name, deps] : map)
	{
		stream << '"' << name << "\":\n[\n";
		for (const auto &dep_name : deps)
		{
			stream << "  \"" << dep_name << "\",\n";
		}
		stream << "]\n\n";
	}

}
