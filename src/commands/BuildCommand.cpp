#include "BuildCommand.hpp"
#include "FieldFile.hpp"
#include "SourceTools.hpp"
#include "code/SourceProcessor.hpp"
#include "BuildCache.hpp"
#include "FileTools.hpp"
#include "Settings.hpp"
#include "utility/Process.hpp"
#include "BuildTools.hpp"
#include "utility/FileStats.hpp"
#include "misc/ContainerTools.hpp"

#include <chrono>
#include <set>
#include <thread>
#include <mutex>
#include <utility/ThreadBatcher.hpp>

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
static void dump_build_args(const std::vector<build_tools::ExecuteParameter> &list, const FilePath &cache_folder);
static void write_auto_generated_temp_header(std::ostream &stream);

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

    if (m_rebuild)
    {
      Logger::notify("rebuilding project: deleting output directory & cache directory");
      build_tools::DeleteBuildDir(this->m_project);
    }

    // is false if we are rebuilding
    m_loaded_build_cache = !m_rebuild && _load_build_cache();
    const bool running_rebuild = this->is_running_rebuild();
    // rebuilding is not normal
    const bool running_normal_build = !running_rebuild;

    if (running_normal_build && !m_loaded_build_cache)
    {
      build_tools::DeleteBuildCache(this->m_project);
    }

    m_project.get_output().ensure_available();

    // create the source processor
    m_src_processor = SourceProcessor();
    _build_source_processor();

    m_src_processor.process();

    const std::map<FilePath, FilePath> input_output_map = this->_gen_io_map();
    const std::set<FilePath> current_source_files = container_tools::keys(input_output_map);
    const std::set<FilePath> current_object_files =
      container_tools::values_set(input_output_map);
    std::set<FilePath> rebuild_files;

    if (m_loaded_build_cache)
    {
      build_tools::DeleteUnusedObjFiles(
        m_build_cache.extract_compiled_paths(),
        current_object_files
      );
    }

    this->m_new_cache = m_build_cache;
    m_new_cache.build_time = t::Now_ms();

    build_tools::SetupHashes(m_new_cache, m_project, m_current_build_cfg);

    Logger::verbose(
      "build cache hashes: new[%llX, %llX]  old[%llX, %llX]\n",
      m_new_cache.build_hash, m_new_cache.config_hash,
      m_build_cache.build_hash, m_build_cache.config_hash
    );

    if (this->_try_rebuild_setup())
    {
      rebuild_files = current_source_files;
    }
    else
    {
      // add files that have changed from last build
      _add_changed_files(rebuild_files);

      // add files with no compiled object file
      _add_uncompiled_files(input_output_map, !is_running_rebuild(), rebuild_files);
    }

    std::vector<build_tools::ExecuteParameter> build_args{};

    for (const FilePath &source_path : rebuild_files)
    {
      const FilePath &output_path = input_output_map.at(source_path);
      const hash_t &hash = m_src_processor.get_file_hash(source_path);

      const FileStats file_stats = {source_path};

      BuildCache::FileRecord record;
      record.hash = hash;
      record.output_path = output_path;
      record.source_write_time = file_stats.last_write_time.count();

      m_new_cache.override_old_source_record(
        source_path, record
      );


      Logger::verbose(
        "adding \"%s\" -> \"%s\" to the build force",
        source_path.c_str(), output_path.c_str()
      );

      const SourceFileType file_type = build_tools::DefaultSourceFileTypeForFilePath(source_path);
      Logger::verbose(
        "source file type for '%s': %s",
        source_path.c_str(),
        build_tools::GetSourceFileTypeName(file_type)
      );

      m_current_build_cfg->build_arguments(
        build_args.emplace_back().args,
        source_path.get_text(),
        output_path.get_text(),
        file_type
      );

      build_args.back().name = source_path.c_str();
      build_args.back().flags |= build_tools::eExcFlag_Printout;
      build_args.back().out = &std::cout;
    }

    m_project.get_output().ensure_available();

    Logger::debug("building objects:");

    // diverting build output to independent stream, avoid parallel output shenanigans
    std::unique_ptr<std::ostringstream[]> build_output_streams{new std::ostringstream[build_args.size()]{}};

    // setting up
    for (size_t i = 0; i < build_args.size(); i++)
    {
      build_args[i].out = build_output_streams.get() + i;
    }

    // building
    const std::vector<int> build_results = ExecuteBuild({build_args.data(), build_args.size()});

    // unloading
    for (size_t i = 0; i < build_args.size(); i++)
    {
      Logger::notify("'%s' output: ", build_args[i].name.c_str());

      Logger::raise_indent();
      Logger::write_raw("%s", build_output_streams.get()[i].str().c_str());
      Logger::lower_indent();
    }

    // clearing
    build_output_streams.reset(nullptr);

    Logger::debug("writing build cache");
    m_new_cache.fix_file_records();
    m_build_cache = m_new_cache;
    _write_build_info();

    const size_t compile_failures = \
      build_results.size() - std::count(build_results.begin(), build_results.end(), 0);
    if (compile_failures > 0)
    {
      for (size_t i = 0; i < build_results.size(); i++)
      {
        if (build_results[i] == 0)
        {
          continue;
        }
        const auto &execute_params = build_args[i];
        Logger::verbose("process titled '%s' failed with error code: %d", execute_params.name.c_str(), build_results[i]);
      }

      Logger::warning(
        "Linking will fail: %llu out of %llu builds have failed",
        compile_failures,
        build_results.size()
      );
    }

    const bool intermidiate_build_all_success = (compile_failures == 0);

    const char *force_linking_setting_name = "force_linking";

    // forces linking even if one or more intermediates failed building, overrides `always_link_build` if `true`
    const bool force_linking = Settings::Get(force_linking_setting_name, false).get_bool();

    // always link building even if no intermediates (object files) changed (lib/config changes are handled above)
    const bool always_link_build = Settings::Get("always_link_build", true).get_bool();

    // no intermediates required rebuilding 
    const bool all_intermediates_upto_date = rebuild_files.empty() && build_results.empty();

    // should we link
    const bool link_upto_date_intermediates = always_link_build || !all_intermediates_upto_date;

    const bool linking_unnecessary = all_intermediates_upto_date && !always_link_build && !force_linking;

    build_tools::ExecuteParameter link_param = {};

    if (linking_unnecessary)
    {
      Logger::notify(
        "All intermediate[s] upto-date, no linking required (flag `always_link_build` is false)."
      );
    }
    else if (intermidiate_build_all_success || force_linking)
    {
      Logger::notify("preparing the final stage (linking)");
      std::vector<StrBlob> link_input_files = _get_linker_inputs(input_output_map);

      std::vector<SourceFileType> source_file_types{};

      for (const FilePath &source_path : current_source_files) {
        source_file_types.emplace_back(build_tools::DefaultSourceFileTypeForFilePath(source_path));
      }

      const SourceFileType dominate_file_type = build_tools::GetDominantSourceType(
        {source_file_types.data(), source_file_types.size()}
      );

      Logger::verbose(
        "linker dominate source file type: %s",
        build_tools::GetSourceFileTypeName(dominate_file_type)
      );

      const FilePath output_filepath = m_project.get_output().get_result_path().resolve();

      m_current_build_cfg->build_link_arguments(
        link_param.args,
        {link_input_files.data(), link_input_files.size()},
        output_filepath.get_text(),
        dominate_file_type
      );

      link_param.name = "binary";
      link_param.flags |= build_tools::eExcFlag_Printout;

      std::ostringstream link_out{};
      link_param.out = &link_out;

      const auto linking_processes_results = ExecuteBuild({&link_param, 1});
      const int link_result = linking_processes_results[0];

      Logger::raise_indent();
      Logger::write_raw("%s", link_out.str().c_str());
      Logger::lower_indent();

      if (link_result != EOK)
      {
        Logger::error("Linking failed: error=%s [%d]", GetErrorName(link_result), link_result);
      }
    }
    else
    {
      Logger::error("Linking not viable: intermidiate file/object compilation failed...");
      Logger::note(
        "* you can set the setting field '%s' to 'true' to proceed linking even if the compilation step failed",
        force_linking_setting_name
      );
    }


    build_args.push_back(link_param);

    // if 'no_cache' or 'clear_cache', the cache will be deleted
    if (Settings::Get("no_cache", Settings::Get("clear_cache", false)))
    {
      Logger::notify("Applying 'no_cache' rule: deleting build cache post finalizing");
      build_tools::DeleteBuildCache(m_project);
    }

    // dump_dep_map(dependencies, m_project.get_output().dir);
    dump_build_args(build_args, m_project.get_output().dir.field());


    return Error::Ok;
  }

  Error BuildCommand::get_help(ArgumentReader &reader, string &out) {
    out
      .append("usage: build [-r/--rebuild] [--resave] [-m=<build mode>/--mode=<build mode>]\n");

    out
      .append("[-r/--rebuild]:\n")
      .append("  rebuilds the current project, deleting all the cached object files (+ it's folder)\n");

    out
      .append("[--resave]:\n")
      .append("  saves the project bake to the project file (for fixing project files)\n")
      .append("  project file is the '.bgnu' in the current working directory\n");

    out
      .append("[-m=<build mode>/--mode=<build mode>]:\n")
      .append("  sets the build mode (i.e. 'release' or 'debug')\n")
      .append("  defaults to 'debug'\n")
      .append("  if no build configuration has the given '<build mode>' name, an error is thrown\n");

    return Error();
  }

  bool BuildCommand::is_running_rebuild() const {
    return m_rebuild || !m_loaded_build_cache;
  }

  void BuildCommand::_add_uncompiled_files(const BuildCommand::IOMap &input_output_map, const bool running_normal_build, std::set<FilePath> &rebuild_files) {
    for (const auto &in_out : input_output_map)
    {
      if (!in_out.second.is_file())
      {
        if (running_normal_build)
        {
          Logger::debug(
            "rebuilding \"%s\": compiled object at \"%s\" not found",
            in_out.first.c_str(),
            in_out.second.c_str()
          );
        }

        rebuild_files.insert(in_out.first);
      }
    }
  }

  void BuildCommand::_add_changed_files(std::set<FilePath> &rebuild_files) const {
    const SourceProcessor::file_change_list changed_files = \
      m_src_processor.gen_file_change_table();

    std::for_each(
      changed_files.begin(),
      changed_files.end(),
      [&rebuild_files](const pair<FilePath, hash_t> &input) {
        Logger::debug("rebuilding \"%s\": cached object file invalidated", input.first.c_str());
        rebuild_files.insert(input.first);
      }
    );
  }

  bool BuildCommand::_try_rebuild_setup() {
    if (is_running_rebuild())
    {
      return true;
    }

    if (!m_new_cache.is_compatible_with(m_build_cache))
    {
      Logger::notify("build cache has been invalidated, rebuilding...");
      build_tools::DeleteBuildCache(m_project);
      return true;
    }

    const bool rebuild_old_projects =
      Settings::Get("rebuild_old_projects", FieldVar(true)).get_bool();

    if (rebuild_old_projects && m_build_cache.too_out_dated_with(m_new_cache))
    {
      Logger::notify("build cache is too old, rebuilding...");
      return true;
    }

    return false;
  }

  std::vector<int> BuildCommand::ExecuteBuild(const Blob<const ::build_tools::ExecuteParameter> &params) {
    const bool multithreaded = Settings::Get("build_multithreaded", false).get_bool();
    const int64_t threads_count = Settings::Get("build_jobs_count", (FieldVar::Int)8).get_int();

    if (multithreaded)
    {
      return build_tools::Execute_Multithreaded(params, threads_count);
    }
    return build_tools::Execute(params);

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

    const auto arg_value_prefix_check = [](const Argument &arg) {
      return arg.get_value().starts_with(InputFilePrefix);
      };
    Argument *argument = reader.extract_matching(
      arg_value_prefix_check
    );

    const string default_input_file = "";
    const string &input_file_str = Argument::try_get_value(
      argument, default_input_file
    );

    Logger::verbose("input file string = \"%s\"", input_file_str.c_str());

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

  void BuildCommand::_write_build_info() const {
    const auto data = m_build_cache.write();
    FieldFile::dump(_get_build_cache_path(), data);
  }

  bool BuildCommand::_load_build_cache() {
    const FilePath build_cache_path = _get_build_cache_path();

    if (!build_cache_path.is_file())
    {
      Logger::verbose("no build cache found at '%s'", build_cache_path.c_str());
      return false;
    }


    FieldVar old_build_cache_data;
    try
    {
      old_build_cache_data = FieldFile::load(build_cache_path);
    }
    catch (const std::exception &e)
    {
      Logger::error("failed to load build cache: %s", e.what());
      return false;
    }

    ErrorReport report = {};

    m_build_cache = BuildCache::load(
      FieldDataReader("BuildCache", old_build_cache_data.get_dict()),
      report
    );

    if (report.code != Error::Ok)
    {
      Logger::error(report);
      return false;
    }

    return true;
  }

  void BuildCommand::_build_source_processor() {
    m_src_processor.set_file_records(m_build_cache.file_records);

    for (const FilePath &path : m_current_build_cfg->include_directories.field())
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
      "%s/%s.%llX.o",
      m_project.get_output().cache_dir->c_str(), filepath.name().c_str(), hash
    );

    return FilePath(buf);
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

  write_auto_generated_temp_header(stream);
  stream << "# below is the dependency map of the entire project\n";

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

void dump_build_args(const std::vector<build_tools::ExecuteParameter> &list, const FilePath &cache_folder) {
  FilePath output_path = cache_folder.join_path(".args");
  std::ofstream stream = output_path.stream_write(false);

  const std::streampos start = stream.tellp();

  write_auto_generated_temp_header(stream);
  stream << "# below are the arguments for building the intermediates (object files) and linking/compiling final\n";

  // We can do it by converting 'map' to field var and save it
  // sadly, im too lazy rn

  for (const auto &param : list)
  {
    bool first = true;
    for (const auto &arg : param.args)
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

void write_auto_generated_temp_header(std::ostream &stream) {
  stream << "# This file is temporay and auto generated\n";
  stream << "# made by bgnu " << Settings::GetVersion() << '\n';
}
