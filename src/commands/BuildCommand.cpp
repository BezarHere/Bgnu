#include "BuildCommand.hpp"
#include "FieldFile.hpp"
#include "SourceTools.hpp"
#include "code/SourceProcessor.hpp"
#include "BuildCache.hpp"
#include "FileTools.hpp"
#include "Settings.hpp"
#include "utility/Process.hpp"

#include <chrono>
#include <set>
#include <thread>

const string RebuildArgs[] = {"-r", "--rebuild"};
const string ResaveArgs[] = {"--resave"};

constexpr const char *InputFilePrefix = "-i=";

typedef SourceProcessor::dependency_map dependency_map;
typedef vector<vector<string>> build_args_list;

static inline int64_t get_build_time() {
	using namespace std;
	return (int64_t)chrono::duration_cast<chrono::microseconds>(
		chrono::steady_clock::now().time_since_epoch()
	).count();
}

static void dump_dep_map(const dependency_map &map, const FilePath &cache_folder);
static void dump_build_args(const build_args_list &list, const FilePath &cache_folder);

namespace commands
{
	Error BuildCommand::execute(ArgumentReader &reader) {
		m_report = {};

		this->_load_properties(reader);

		if (!m_project_file.exists())
		{
			Logger::error("No project file exists at \"%s\"", to_cstr(m_project_file.get_text()));

			return Error::Failure;
		}

		if (!m_project_file.is_file())
		{
			Logger::error(
				"The path \"%s\" is to a directory, not a project file",
				to_cstr(m_project_file.get_text())
			);

			return Error::Failure;
		}

		this->_load_project();
		if (m_report.code != Error::Ok)
		{
			Logger::error(
				"Building project reported an err code=%d: %s", (int)m_report.code, to_cstr(m_report.message)
			);
			return m_report.code;
		}

		_setup_build_config(reader);
		if (m_report.code != Error::Ok)
		{
			Logger::error(
				"setting-up the build config reported an error code=%d: %s",
				(int)m_report.code, to_cstr(m_report.message)
			);
			return m_report.code;
		}

		_load_build_cache();

		m_project.get_output().ensure_available();

		// create the source processor
		m_src_processor = SourceProcessor();
		_build_source_processor();

		m_src_processor.process();

		std::map<FilePath, FilePath> input_output_map = this->_gen_io_map();
		std::set<FilePath> rebuild_files;

		// add files that have changed from last build 
		{
			const SourceProcessor::file_change_list changed_files = \
				m_src_processor.gen_file_change_table();

			std::for_each(
				changed_files.begin(),
				changed_files.end(),
				[&rebuild_files](const pair<FilePath, hash_t> &input) {
					rebuild_files.insert(input.first);
				}
			);
		}

		// add files with no compiled object file
		{
			for (const auto &in_out : input_output_map)
			{
				if (!in_out.second.is_file())
				{
					rebuild_files.insert(in_out.first);
				}
			}
		}

		build_args_list build_args{};
		BuildCache new_cache = m_build_cache;
		// new_cache.file_records.clear();

		for (const FilePath &path : rebuild_files)
		{
			const FilePath &output_path = input_output_map.at(path);
			const hash_t &hash = m_src_processor.get_file_hash(path);


			BuildCache::FileRecord record;
			record.build_time = get_build_time();
			record.hash = hash;
			record.source_file = path;
			record.last_source_write_time = get_build_time();

			new_cache.file_records.insert_or_assign(
				output_path, record
			);

			Logger::verbose(
				"adding \"%s\" -> \"%s\" to the build force",
				path.c_str(), output_path.c_str()
			);

			m_current_build_cfg->build_arguments(
				build_args.emplace_back(),
				path.get_text(),
				output_path.get_text(),
				SourceFileType::CPP
			);
		}

		build_args_list link_args = {};
		std::vector<StrBlob> link_input_files = this->_get_linker_inputs(input_output_map);

		const FilePath output_filepath = m_project.get_output().get_result_path();

		m_current_build_cfg->build_link_arguments(
			link_args.emplace_back(),
			{link_input_files.data(), link_input_files.size()},
			output_filepath.get_text()
		);

		new_cache.config_hash = m_project.hash();
		new_cache.build_hash = m_current_build_cfg->hash();

		m_build_cache = new_cache;

		m_project.get_output().ensure_available();
		_write_build_cache();

		Logger::debug("building objects:");
		this->_execute_build(build_args);

		Logger::debug("linking executable:");
		this->_execute_build(link_args);

		// TODO: link args with all the input files

		// dump_dep_map(dependencies, m_project.get_output().dir);
		dump_build_args(build_args, m_project.get_output().dir);


		return Error::Ok;
	}

	FilePath BuildCommand::_default_filepath() {
		return FilePath::get_working_directory().join_path(".bgnu");
	}

	void BuildCommand::_load_properties(ArgumentReader &reader) {
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

		m_project_file = input_file_str.empty() ? _default_filepath() : FilePath(input_file_str);

		Logger::verbose("input path = \"%s\"", to_cstr(m_project_file.get_text()));
	}

	void BuildCommand::_load_project() {
		m_report = {};

		if (!m_project_file.is_file())
		{
			m_report = {
				Error::FileNotFound,
				format_join("Project file at '", m_project_file,"' not found!")
			};
			return;
		}

		const FilePath project_dir = m_project_file.parent();
		const FieldVar project_file_data = FieldFile::load(m_project_file);

		if (project_file_data.is_null())
		{
			m_report = {
				Error::NullData,
				format_join("Null project data file at '", m_project_file,"'; this shouldn't happen, bug?")
			};
			return;
		}

		if (project_file_data.get_dict().empty())
		{
			m_report = {
				Error::NoData,
				format_join("Empty project data file at '", m_project_file,"', check your file!")
			};
			return;
		}

		m_project = Project::from_data(project_file_data.get_dict(), m_report);
		m_project.source_dir = project_dir;


		if (m_project.get_build_configs().empty())
		{
			m_report = {Error::NoConfig, "Project has no build configurations available"};
			return;
		}
	}

	void BuildCommand::_setup_build_config(ArgumentReader &reader) {
		constexpr auto mode_matcher = [](const Argument &arg) {
			return arg.get_value().starts_with("-m=") || arg.get_value().starts_with("--mode=");};

		m_report = {};
		Argument *arg = reader.extract_matching(mode_matcher);

		if (arg == nullptr)
		{
			m_current_build_cfg_name = "debug";
			Logger::verbose("no build mode specified, use default '%s'", m_current_build_cfg_name.c_str());
		}
		else
		{
			const size_t equal_pos = arg->get_value().find('=');

			if (equal_pos == string::npos)
			{
				m_report = {
				Error::Failure,
				"found an argument with a specified build mode, but it has no equality (BUG)"
				};
				return;
			}

			m_current_build_cfg_name = arg->get_value().substr(equal_pos + 1);
			if (m_current_build_cfg_name.starts_with('"') && m_current_build_cfg_name.ends_with('"'))
			{
				m_current_build_cfg_name = m_current_build_cfg_name.substr(1, m_current_build_cfg_name.size() - 2);
			}
		}

		Logger::note("building '%s' config", m_current_build_cfg_name.c_str());

		const auto config_pos = m_project.get_build_configs().find(m_current_build_cfg_name);

		if (config_pos == m_project.get_build_configs().end())
		{
			m_report = {
				Error::NoConfig, format_join("No config found named '", m_current_build_cfg_name, "'")
			};
			return;
		}

		m_current_build_cfg = &config_pos->second;
	}

	void BuildCommand::_write_build_cache() const {
		const auto data = m_build_cache.write();
		FieldFile::dump(_get_build_cache_path(), data);
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

	void BuildCommand::_build_source_processor() {
		m_src_processor.set_file_records(m_build_cache.file_records);

		for (const FilePath &path : m_current_build_cfg->include_directories)
		{
			m_src_processor.included_directories.emplace_back(
				path.get_text(),
				m_project.source_dir.get_text()
			);
		}


		for (const auto &path : m_project.get_source_files())
		{
			m_src_processor.add_file({path, SourceFileType::None});
		}
	}

	BuildCommand::IOMap BuildCommand::_gen_io_map() const {
		std::map<FilePath, FilePath> result{};
		for (const auto &inputs : m_src_processor.get_inputs())
		{
			result.emplace(
				inputs.path, _get_output_filepath(inputs.path, m_src_processor.get_file_hash(inputs.path))
			);
		}

		return result;
	}

	FilePath BuildCommand::_get_output_filepath(const FilePath &filepath, const hash_t &hash) const {
		string_char buf[FilePath::MaxPathLength + 1] = {};
		sprintf_s(
			buf,
			"%s/%s.%X.o",
			m_project.get_output().cache_dir.c_str(), filepath.name().c_str(), hash
		);

		return FilePath(buf);
	}

	void BuildCommand::_execute_build(const vector<vector<string>> &args) {
		const bool flag_multithreaded = Settings::Get("flag_multithreaded", true).get_bool();
		std::thread *threads = flag_multithreaded ? new std::thread[args.size()] : nullptr;
		vector<int> results{};

		const auto cmd = [&results](const string_char *cmd, size_t index)
			{
				Logger::verbose("\nrunning '%s' index %llu\n", cmd, index);
				Process process = {cmd};
				const int result = process.start();
				Logger::verbose("process '%s' returned %d\n", cmd, result);
				results.push_back(result);
				return result;
			};


		for (size_t i = 0; i < args.size(); i++)
		{
			std::ostringstream oss;

			// TODO: use the file's source type
			oss << BuildConfiguration::get_compiler_name(m_current_build_cfg->compiler_type, SourceFileType::CPP);
			oss << ' ';

			for (const string &str : args[i])
			{
				oss << str << ' ';
			}

			const string command_line = oss.str();
			std::cout << "running '" << command_line << "'\n";

			if (flag_multithreaded)
			{
				threads[i] = std::thread(
					cmd, command_line.c_str(), i
				);
			}
			else
			{
				cmd(command_line.c_str(), i);
			}
		}

		if (flag_multithreaded)
		{
			for (size_t i = 0; i < args.size(); i++)
			{
				threads[i].join();
			}
		}

		delete[] threads;
	}

	std::vector<StrBlob> BuildCommand::_get_linker_inputs(const IOMap &io_map) {
		std::vector<StrBlob> result = {};

		for (const auto &[src_path, obj_path] : io_map)
		{
			result.emplace_back(obj_path.get_text());
		}

		return result;
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

	Logger::verbose("dumped the build argument list: size=%lld bytes to '%s'", stream.tellp() - start, output_path.c_str());
}
