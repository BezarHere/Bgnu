#include "BuildCommand.hpp"
#include "FieldFile.hpp"
#include "SourceTools.hpp"
#include "code/SourceProcessor.hpp"
#include "BuildCache.hpp"
#include "FileTools.hpp"

const string RebuildArgs[] = {"-r", "--rebuild"};
const string ResaveArgs[] = {"--resave"};

constexpr const char *InputFilePrefix = "-i=";

typedef SourceProcessor::dependency_map dependency_map;
typedef vector<vector<string>> build_args_list;

static void dump_dep_map(const dependency_map &map, const FilePath &cache_folder);
static void dump_build_args(const build_args_list &list, const FilePath &cache_folder);

namespace commands
{
	Error BuildCommand::execute(ArgumentReader &reader) {

		m_rebuild = Argument::try_use(
			reader.extract_any(Blob<const string>(RebuildArgs))
		);

		m_resave = Argument::try_use(
			reader.extract_any(Blob<const string>(ResaveArgs))
		);

		Logger::verbose("rebuild = %s", to_boolalpha(m_rebuild));
		Logger::verbose("resave = %s", to_boolalpha(m_resave));

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

		// FieldFile::dump(string(input_file_path) + string("-out"), project_file_data.get_dict());

		m_report = {};
		m_project = Project::from_data(project_file_data.get_dict(), m_report);
		m_project.source_dir = project_dir;

		if (m_report.code != Error::Ok)
		{
			Logger::error(
				"Building project reported an err code=%d: %s", (int)m_report.code, to_cstr(m_report.message)
			);
			return m_report.code;
		}

		if (m_project.get_build_configs().empty())
		{
			Logger::error(
				"No build configurations are available"
			);
			return Error::NoConfig;
		}

		const BuildConfiguration &build_cfg = m_project.get_build_configs().at("debug");

		_load_build_cache();


		m_project.get_output().ensure_available();

		std::map<string, vector<string>> dependencies{};
		SourceProcessor processor;
		processor.file_records = m_build_cache.file_records;

		for (const auto &path : m_project.get_source_files())
		{
			processor.add_file({path, SourceFileType::None});
		}

		processor.process();

		const SourceProcessor::file_change_list changed_files = processor.gen_file_change_table();
		build_args_list build_args{};

		for (const auto &[path, hash] : changed_files)
		{
			FilePath output_path = SourceTools::get_intermediate_filepath(
				path,
				m_project.get_output().cache_dir
			);

			output_path = FilePath(output_path.c_str() + string(".o"));

			build_cfg.build_arguments(
				build_args.emplace_back(),
				path.get_text(),
				output_path.get_text(),
				SourceFileType::CPP
			);
		}

		BuildCache new_cache = m_build_cache;
		m_build_cache = new_cache;

		// TODO: link args with all the input files

		dump_dep_map(dependencies, m_project.get_output().cache_dir);
		dump_build_args(build_args, m_project.get_output().cache_dir);

		return Error::Ok;
	}

	FilePath BuildCommand::_default_filepath() {
		return FilePath::get_working_directory().join_path(".bgnu");
	}

	void BuildCommand::_load_build_cache() {
		const FilePath build_cache_path = _get_build_cache_path();

		if (!build_cache_path.is_file())
		{
			return;
		}

		const FieldVar old_build_cache_data = FieldFile::load(build_cache_path);
		ErrorReport report = {};

		m_build_cache = BuildCache::load(
			FieldDataReader("BuildCache", old_build_cache_data.get_dict()),
			report
		);

		if (report.code != Error::Ok)
		{
			Logger::error(report);
		}
	}
}

void dump_dep_map(const dependency_map &map, const FilePath &cache_folder) {
	FilePath output_path = cache_folder.join_path(".deps");
	std::ofstream stream = output_path.stream_write(false);

	const std::streampos start = stream.tellp();

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

	Logger::verbose("dumped the dependency map: size=%lld bytes", stream.tellp() - start);
}

void dump_build_args(const build_args_list &list, const FilePath &cache_folder) {
	FilePath output_path = cache_folder.join_path(".args");
	std::ofstream stream = output_path.stream_write(false);

	const std::streampos start = stream.tellp();

	// We can do it by converting 'map' to field var and save it
	// sadly, im too lazy rn

	for (const auto &args : list)
	{
		bool first = true;
		for (const auto &arg : args)
		{
			if (!first)
			{
				stream << ' ';
			}
			first = false;
			stream << arg;
		}
		stream << '\n';
	}

	Logger::verbose("dumped the build argument list: size=%lld bytes", stream.tellp() - start);
}
