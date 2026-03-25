#include "ProjectService.hpp"

#include <algorithm>
#include <cmath>
#include <memory>
#include <memory_resource>
#include <utility>
#include <vector>

#include "Argument.hpp"
#include "BuildTools.hpp"
#include "FieldFile.hpp"
#include "Logger.hpp"
#include "Project.hpp"
#include "Settings.hpp"
#include "code/SourceProcessor.hpp"
#include "misc/ContainerTools.hpp"
#include "misc/Error.hpp"
#include "misc/Time.hpp"
#include "utility/FileStats.hpp"

BuildStep ProjectService::s_current_step = BuildStep::None;
ArgumentSource ProjectService::s_arguments = {};

std::unique_ptr<Project> ProjectService::s_project = nullptr;
string ProjectService::s_current_config_name = "";
const BuildConfiguration *ProjectService::s_current_config = nullptr;

vector<FilePath> ProjectService::s_source_files = {};
vector<FilePath> ProjectService::s_hash_mismatched_source_files = {};
vector<FilePath> ProjectService::s_unrecorded_source_files = {};
vector<FilePath> ProjectService::s_hanging_source_files = {};
vector<FilePath> ProjectService::s_compile_needed_source_files = {};

vector<build_tools::BuildCommandInfo>
    ProjectService::s_total_build_commands = {};
vector<build_tools::BuildCommandInfo>
    ProjectService::s_used_build_commands = {};
vector<int> ProjectService::s_source_build_result_codes = {};

std::map<FilePath, FilePath> ProjectService::s_source_io_map = {};
std::map<FilePath, hash_t> ProjectService::s_source_files_hashes_map = {};
std::map<FilePath, hash_t> ProjectService::s_obj_files_hashes_map = {};
std::map<FilePath, hash_t> ProjectService::s_source2obj_files_hashes_map = {};

build_tools::BuildCommandInfo ProjectService::s_linking_build_cmd = {};
int ProjectService::s_linking_result_code = 0;

FilePath ProjectService::s_project_file = {};
FilePath ProjectService::s_build_directory = {};

BuildCache ProjectService::s_current_cache = {};
BuildCache ProjectService::s_updated_cache = {};

bool ProjectService::s_cache_loaded = false;
bool ProjectService::s_forced_rebuild = false;
bool ProjectService::s_resave_required = false;
bool ProjectService::s_hash_mismatched = false;

constexpr string RebuildArgs[] = { "-r", "--rebuild" };
constexpr string ResaveArgs[] = { "--resave" };

constexpr const char *InputFilePrefix = "-i=";

void ProjectService::Clear() {
  s_current_step = BuildStep::None;

  s_project = nullptr;
  s_current_config_name = "";
  s_current_config = nullptr;

  s_source_files = {};
  s_hash_mismatched_source_files = {};
  s_unrecorded_source_files = {};
  s_compile_needed_source_files = {};

  s_total_build_commands = {};
  s_used_build_commands = {};
  s_source_build_result_codes = {};

  s_source_io_map = {};
  s_source_files_hashes_map = {};
  s_obj_files_hashes_map = {};
  s_source2obj_files_hashes_map = {};

  s_linking_build_cmd = {};
  s_linking_result_code = 0;

  s_project_file = {};
  s_build_directory = {};

  s_current_cache = {};
  s_updated_cache = {};

  s_cache_loaded = false;
  s_forced_rebuild = false;
  s_resave_required = false;
  s_hash_mismatched = false;
}

Error ProjectService::ExecuteStep(BuildStep step) {
  Error result = ExecuteStep_Inner(step);
  Logger::debug("Running build step '%s' resulted in %s",
                GetStepName(step),
                GetErrorName(result));
  return result;
}

std::pair<Error, BuildStep> ProjectService::ExecuteStepsTo(
    BuildStep final_step) {
  constexpr BuildStep first_step = BuildStep::ArgsSetup;
  constexpr BuildStep last_step = BuildStep::PostLinking;

  if ((int)final_step > (int)last_step)
  {
    final_step = last_step;
  }

  for (int i = (int)first_step; i <= (int)final_step; i++)
  {
    Error result = ExecuteStep(BuildStep(i));
    if (result != Error::Ok)
    {
      return { result, BuildStep(i) };
    }
  }

  return { Error::Ok, BuildStep::None };
}

void ProjectService::SetArguments(ArgumentSource args) {
  s_arguments = args;
  s_arguments.simplify();
}

Error ProjectService::SetupArgs() {
  Clear();

  s_forced_rebuild = Argument::try_use(
      s_arguments.extract_any(Blob<const string>(RebuildArgs)));

  s_resave_required = Argument::try_use(
      s_arguments.extract_any(Blob<const string>(ResaveArgs)));

  Logger::verbose("rebuild = %s", to_boolalpha(s_forced_rebuild));
  Logger::verbose("resave = %s", to_boolalpha(s_resave_required));

  const auto arg_value_prefix_check = [](const Argument &arg) {
    return arg.get_value().starts_with(InputFilePrefix);
  };
  Argument *argument = s_arguments.extract_matching(arg_value_prefix_check);

  const string default_input_file = "";
  const string &input_file_str =
      Argument::try_get_value(argument, default_input_file);

  Logger::verbose("input file string = \"%s\"", input_file_str.c_str());

  s_project_file = input_file_str.empty()
                       ? build_tools::DefaultProjectFilePath()
                       : FilePath(input_file_str);

  Logger::verbose("input path = \"%s\"", to_cstr(s_project_file.get_text()));

  ErrorReport err = SetupConfigArgs(s_arguments);
  if (err.code != Error::Ok)
  {
    Logger::error("setting-up the build config reported an error code=%d: %s",
                  (int)err.code,
                  to_cstr(err.message));
    return err.code;
  }

  if (Logger::is_verbose())
  {
    for (const auto &arg : s_arguments.get_args())
    {
      Logger::verbose("+ has arg: \"%s\" [%s]",
                      arg.get_value().c_str(),
                      arg.is_used() ? "U" : " ");
    }
  }
  return Error::Ok;
}

Error ProjectService::SetupProject() {
  if (!s_project_file.exists())
  {
    Logger::error("No project file exists at \"%s\"",
                  to_cstr(s_project_file.get_text()));

    return Error::Failure;
  }

  if (!s_project_file.is_file())
  {
    Logger::error("The path \"%s\" is to a directory, not a project file",
                  to_cstr(s_project_file.get_text()));

    return Error::Failure;
  }

  ErrorReport err = LoadProject();
  if (err)
  {
    Logger::error("Building project reported an err code=%d: %s",
                  (int)err.code,
                  to_cstr(err.message));
    return err.code;
  }

  err = SetupConfig();
  if (err)
  {
    Logger::error("Failed to setup current build configuration:%d: %s",
                  (int)err.code,
                  to_cstr(err.message));
    return err.code;
  }

  return Error::Ok;
}

Error ProjectService::LoadCaches() {
  if (s_forced_rebuild)
  {
    s_cache_loaded = false;
    return Error::Ok;
  }

  ErrorReport err = ReadBuildCache();
  if (err && err.code != Error::FileNotFound)
  {
    return err.code;
  }

  s_cache_loaded = true;
  UpdateHashMismatchFlag();
  return Error::Ok;
}

Error ProjectService::ProcessPrebuild() {
  if (ShouldRebuild())
  {
    build_tools::DeleteBuildCache(*s_project);
  }

  return Error::Ok;
}

Error ProjectService::LoadSourceInfo() {
  SourceProcessor processor = {};
  ErrorReport err = BuildSourceProcessor(processor);
  if (err)
  {
    Logger::error(err);
    return err.code;
  }

  processor.process();
  build_tools::DumpDependencyMap(processor.get_dependency_info_map(),
                                 s_project->get_output().dir.field());

  err = SetupSourceProperties(processor);
  if (err)
  {
    return err.code;
  }

  return Error::Ok;
}

Error ProjectService::RemoveUnusedCacheFiles() {
  if (s_cache_loaded)
  {
    build_tools::DeleteUnusedObjFiles(
        s_current_cache.extract_compiled_paths(),
        container_tools::values_set(s_source_io_map));
  }

  return Error::Ok;
}

Error ProjectService::UpdateBuildCache() {
  s_updated_cache = s_current_cache;
  s_updated_cache.build_time = t::Now_ms();
  build_tools::SetupHashes(s_updated_cache, *s_project, s_current_config);

  Logger::verbose("## build cache hashes: new[%llX, %llX]  old[%llX, %llX] ##",
                  s_updated_cache.build_hash,
                  s_updated_cache.config_hash,
                  s_current_cache.build_hash,
                  s_current_cache.config_hash);
  return Error::Ok;
}

Error ProjectService::PopulateCompileNeededFiles() {
  if (ShouldRebuild())
  {
    Logger::note("** rebuilding project: deleting output & cache directory **");
    s_compile_needed_source_files = s_source_files;
    build_tools::DeleteBuildDir(*s_project);
    return Error::Ok;
  }
  // add files that have changed from last build
  ErrorReport err = PopulateSourceFilesToBuild();

  if (err)
  {
    Logger::error(err);
    return err.code;
  }

  // add files with no compiled object file
  err = PopulateSourceFilesToBuild();

  if (err)
  {
    Logger::error(err);
    return err.code;
  }

  return Error::Ok;
}

Error ProjectService::PopulateBuildCommands() {
  for (const FilePath &source_path : s_source_files)
  {
    ErrorReport err =
        SetupBuildCommand(source_path, s_total_build_commands.emplace_back());
    if (err)
    {
      Logger::error(err);
      return err.code;
    }

    const auto find_result = std::find(s_compile_needed_source_files.begin(),
                                       s_compile_needed_source_files.end(),
                                       source_path);
    // if this source file should be rebuilt
    if (find_result != s_compile_needed_source_files.end())
    {
      s_used_build_commands.emplace_back(s_total_build_commands.back());
    }
  }

  return Error::Ok;
}

Error ProjectService::PostBuildCommandsStep() {
  s_project->get_output().ensure_available();

  if (s_project->should_handle_clangd())
  {
    Logger::debug("Written clangd compile flags");
    build_tools::TryCreateClangdCompileFlagsFile(*s_current_config,
                                                 s_build_directory);

    std::vector<string> cmds = {};
    std::vector<string> names = {};

    for (size_t i = 0; i < s_total_build_commands.size(); i++)
    {

      cmds.emplace_back(build_tools::JoinArguments(
          s_total_build_commands[i].args.data(),
          s_total_build_commands[i].args.size(),
          build_tools::IsAllowedArgumentForClangdFiles));
      names.emplace_back(s_build_directory.relative_to(
          s_total_build_commands[i].in_path.resolved_copy()));
    }

    Logger::debug("Written clangd compile commands");
    build_tools::WriteClangdCompileCommandsFile(cmds.data(),
                                                names.data(),
                                                cmds.size(),
                                                s_build_directory);
  }

  return Error::Ok;
}

Error ProjectService::DispatchBuildProcesses() {

  s_source_build_result_codes.clear();
  s_source_build_result_codes.resize(s_used_build_commands.size());

  Logger::raise_indent();
  ErrorReport err = DispatchBuildCommands(s_source_build_result_codes.data());
  Logger::lower_indent();

  if (err)
  {
    Logger::error(err);
    return err.code;
  }

  return {};
}

Error ProjectService::ReapplyUpdatedBuildCache() {
  LoadObjectHashesToUpdatedCache();
  s_updated_cache.fix_file_records();
  s_current_cache = s_updated_cache;
  const auto data = s_current_cache.write();
  FieldFile::dump(GetBuildCachePath(), data);
  return Error::Ok;
}

Error ProjectService::LinkBuiltFiles() {
  const bool intermidiate_build_all_success = IsBuildSuccessful();

  constexpr char force_linking_setting_name[] = "force_linking";

  // forces linking even if one or more intermediates failed building, overrides
  // `always_link_build` if `true`
  const bool force_linking =
      Settings::Get(force_linking_setting_name, false).get_bool();

  // always link building even if no intermediates (object files) changed
  // (lib/config changes are handled above)
  const bool always_link_build =
      Settings::Get("always_link_build", true).get_bool();

  // no intermediates required rebuilding
  // no failures and no success => No files needed a rebuild
  const bool all_intermediates_upto_date =
      intermidiate_build_all_success && GetBuildSuccessCount() == 0;

  const bool linking_necessary =
      !all_intermediates_upto_date ||
      (all_intermediates_upto_date && always_link_build) || force_linking;

  s_linking_result_code = -1;
  if (!linking_necessary)
  {
    s_linking_result_code = EOK;
    Logger::notify(
        "All intermediate[s] upto-date, no linking required (flag "
        "`always_link_build` is "
        "false).");
    return Error::Ok;
  }

  if (!force_linking && !intermidiate_build_all_success)
  {
    Logger::error(
        "Linking not viable: intermidiate file/object compilation failed...");
    Logger::note(
        "* you can set the setting field '%s' to 'true' to proceed linking "
        "even if the "
        "compilation step failed",
        force_linking_setting_name);
    return Error::Failure;
  }

  const FilePath output_filepath =
      s_project->get_output().get_result_path().resolve();

  const bool linked_by_force = !intermidiate_build_all_success;
  Logger::notify("preparing the final stage (linking)%s",
                 linked_by_force ? " [FORCED]" : "");

  std::vector<StrBlob> link_input_files = GenerateLinkerInputs();

  std::vector<SourceFileType> source_file_types{};

  for (const FilePath &source_path : s_compile_needed_source_files)
  {
    source_file_types.emplace_back(
        build_tools::DefaultSourceFileTypeForFilePath(source_path));
  }

  const SourceFileType dominate_file_type = build_tools::GetDominantSourceType(
      { source_file_types.data(), source_file_types.size() });

  Logger::verbose("linker dominate source file type: %s",
                  build_tools::GetSourceFileTypeName(dominate_file_type));

  s_linking_build_cmd = {};
  s_current_config->build_link_arguments(
      s_linking_build_cmd.args,
      { link_input_files.data(), link_input_files.size() },
      output_filepath.get_text(),
      dominate_file_type);

  s_linking_build_cmd.name = "binary";
  s_linking_build_cmd.flags |= build_tools::eExcFlag_Printout;

  std::ostringstream link_out{};
  s_linking_build_cmd.out = &link_out;

  ErrorReport err =
      ExecuteBuildCommands(&s_linking_build_cmd, &s_linking_result_code, 1);
  if (err)
  {
    Logger::error(err);
    return err.code;
  }

  Logger::raise_indent();
  Logger::write_raw("%s", link_out.str().c_str());
  Logger::lower_indent();

  if (s_linking_result_code != EOK)
  {
    Logger::error("Linking failed: error=%s [%d]",
                  GetErrorName(s_linking_result_code),
                  s_linking_result_code);
    return Error::Failure;
  }

  return Error::Ok;
}

Error ProjectService::PostLinkingStep() {
  // if 'no_cache' or 'clear_cache', the cache will be deleted
  if (Settings::Get("no_cache", Settings::Get("clear_cache", false)))
  {
    Logger::notify(
        "Applying 'no_cache' rule: deleting build cache post finalizing");
    build_tools::DeleteBuildCache(*s_project);
  }

  DumpAvailableBuildCommands();
  return Error::Ok;
}

Error ProjectService::DumpAvailableBuildCommands() {
  s_total_build_commands.emplace_back(s_linking_build_cmd);

  DumpBuildCommands(s_total_build_commands.data(),
                    s_total_build_commands.size());

  s_total_build_commands.pop_back();
  return Error::Ok;
}

FilePath ProjectService::GetCompiledOutputPath(const FilePath &path,
                                               hash_t hash) {
  (void)hash;
  string_char buf[FilePath::MaxPathLength + 1] = {};
  snprintf(buf,
           FilePath::MaxPathLength,
           "%s/%s.o",
           s_project->get_output().cache_dir->c_str(),
           path.name().c_str());

  return FilePath(buf);
}

size_t ProjectService::GetBuildFailureCount() {
  LOG_ASSERT(s_used_build_commands.size() ==
             s_source_build_result_codes.size());
  return s_used_build_commands.size() - GetBuildSuccessCount();
}

size_t ProjectService::GetBuildSuccessCount() {
  return std::count(s_source_build_result_codes.begin(),
                    s_source_build_result_codes.end(),
                    0);
}

const char *ProjectService::GetStepName(BuildStep step) {
  switch (step)
  {
  case BuildStep::None:
    return "None";
  case BuildStep::ArgsSetup:
    return "ArgsSetup";
  case BuildStep::ProjectSetup:
    return "ProjectSetup";
  case BuildStep::CacheLoading:
    return "CacheLoading";
  case BuildStep::PrebuildProcessing:
    return "PrebuildProcessing";
  case BuildStep::SourceInfoLoading:
    return "SourceInfoLoading";
  case BuildStep::OldCacheClearing:
    return "OldCacheClearing";
  case BuildStep::UpdatingCache:
    return "UpdatingCache";
  case BuildStep::PopulatingCompileList:
    return "PopulatingCompileList";
  case BuildStep::PopulateBuildCommands:
    return "PopulateBuildCommands";
  case BuildStep::PostBuildCommandsPopulating:
    return "PostBuildCommandsPopulating";
  case BuildStep::BuildCommandsDispatching:
    return "BuildCommandsDispatching";
  case BuildStep::UpdateBuildCacheApplying:
    return "UpdateBuildCacheApplying";
  case BuildStep::Linking:
    return "Linking";
  case BuildStep::PostLinking:
    return "PostLinking";
  }
  return "UNKNOWN";
}

FilePath ProjectService::GetLinkOutputPath() {
  return s_project->get_output().dir->join_path(*s_project->get_output().name);
}

std::string ProjectService::GetDefaultConfigName() {
  if (!s_project)
  {
    return "";
  }

  if (s_project->get_build_configs().empty())
  {
    return "";
  }

  if (s_project->get_build_configs().contains("default"))
  {
    return "default";
  }

  if (s_project->get_build_configs().contains("debug"))
  {
    return "debug";
  }

  if (s_project->get_build_configs().contains("release"))
  {
    return "release";
  }

  return "";
}

void ProjectService::UpdateHashMismatchFlag() {
  if (!s_project || !s_current_config)
  {
    Logger::error(
        "Can't update has mismatch flag if the project or current build config "
        "aren't already set");
    return;
  }

  s_hash_mismatched = !IsProjectHashMatching() || !IsConfigHashMatching();
}

bool ProjectService::IsProjectHashMatching() {
  return s_project->hash() == s_current_cache.build_hash;
}

bool ProjectService::IsConfigHashMatching() {
  return s_current_config->hash() == s_current_cache.config_hash;
}

Error ProjectService::ExecuteStep_Inner(BuildStep step) {
  switch (step)
  {
  case BuildStep::None:
    return Error::Ok;
  case BuildStep::ArgsSetup:
    return SetupArgs();
  case BuildStep::ProjectSetup:
    return SetupProject();
  case BuildStep::CacheLoading:
    return LoadCaches();
  case BuildStep::PrebuildProcessing:
    return ProcessPrebuild();
  case BuildStep::SourceInfoLoading:
    return LoadSourceInfo();
  case BuildStep::OldCacheClearing:
    return RemoveUnusedCacheFiles();
  case BuildStep::UpdatingCache:
    return UpdateBuildCache();
  case BuildStep::PopulatingCompileList:
    return PopulateCompileNeededFiles();
  case BuildStep::PopulateBuildCommands:
    return PopulateBuildCommands();
  case BuildStep::PostBuildCommandsPopulating:
    return PostBuildCommandsStep();
  case BuildStep::BuildCommandsDispatching:
    return DispatchBuildProcesses();
  case BuildStep::UpdateBuildCacheApplying:
    return ReapplyUpdatedBuildCache();
  case BuildStep::Linking:
    return LinkBuiltFiles();
  case BuildStep::PostLinking:
    return PostLinkingStep();
  }
  return Error::InvalidType;
}

ErrorReport ProjectService::LoadProject() {
  s_build_directory = s_project_file.parent();
  const FieldVar project_file_data = FieldFile::load(s_project_file);

  if (project_file_data.is_null())
  {
    return { Error::NullData,
             format_join("Null project data file at '",
                         s_project_file,
                         "'; this shouldn't happen, bug?") };
  }

  if (project_file_data.get_dict().empty())
  {
    return { Error::NoData,
             format_join("Empty project data file at '",
                         s_project_file,
                         "', check your file!") };
  }

  {
    Result<Project> proj = Project::from_data(project_file_data.get_dict());
    if (proj.has_error())
    {
      return proj.to_error_report();
    }

    s_project = std::make_unique<Project>(std::move(*proj));
  }

  s_project->source_dir = s_build_directory;

  if (s_project->get_build_configs().empty())
  {
    return { Error::NoConfig, "Project has no build configurations available" };
  }

  if (Logger::is_verbose())
  {
    for (const auto &config : s_project->get_build_configs())
    {
      Logger::verbose("Found build config: '%s'", config.first.c_str());
    }
  }

  return {};
}

ErrorReport ProjectService::SetupConfigArgs(ArgumentSource &src) {
  constexpr auto mode_matcher = [](const Argument &arg) {
    return arg.get_value().starts_with("-m=") ||
           arg.get_value().starts_with("--mode=");
  };

  Argument *arg = src.extract_matching(mode_matcher);

  if (arg == nullptr)
  {
    s_current_config_name = "";
    Logger::verbose("no build mode specified!");
  }
  else
  {
    const size_t equal_pos = arg->get_value().find('=');
    arg->mark_used();

    if (equal_pos == string::npos)
    {
      return { Error::Failure,
               "found an argument with a specified build mode, but it has no "
               "equality (BUG?)" };
      ;
    }

    s_current_config_name = arg->get_value().substr(equal_pos + 1);
    if (s_current_config_name.starts_with('"') &&
        s_current_config_name.ends_with('"'))
    {
      s_current_config_name =
          s_current_config_name.substr(1, s_current_config_name.size() - 2);
    }
  }

  Logger::note("defined used config: '%s'", s_current_config_name.c_str());
  return {};
}

ErrorReport ProjectService::SetupConfig() {
  if (s_current_config_name.empty())
  {
    s_current_config_name = GetDefaultConfigName();
    Logger::notify("No config set, Defaulting to '%s'",
                   s_current_config_name.c_str());
  }

  const auto config_pos =
      s_project->get_build_configs().find(s_current_config_name);

  if (config_pos == s_project->get_build_configs().end())
  {
    return {
      Error::NoConfig,
      format_join("No config found named '", s_current_config_name, "'")
    };
  }

  s_current_config = &config_pos->second;
  return {};
}

ErrorReport ProjectService::ReadBuildCache() {
  const FilePath build_cache_path = GetBuildCachePath();

  if (!build_cache_path.is_file())
  {
    Logger::verbose("no build cache found at '%s'", build_cache_path.c_str());
    return { Error::FileNotFound, "Build cache not found" };
  }

  FieldVar old_build_cache_data;
  try
  { old_build_cache_data = FieldFile::load(build_cache_path); }
  catch (const std::exception &e)
  {
    Logger::error("failed to load build cache: %s", e.what());
    return { Error::FileNotFound, e.what() };
  }

  ErrorReport report = {};

  s_current_cache = BuildCache::load(
      FieldDataReader("BuildCache", old_build_cache_data.get_dict()),
      report);

  if (report.code != Error::Ok)
  {
    Logger::error(report);
    return report;
  }

  return {};
}

ErrorReport ProjectService::BuildSourceProcessor(SourceProcessor &processor) {
  processor.set_file_records(s_current_cache.file_records);

  for (const FilePath &path : s_current_config->include_directories.field())
  {
    processor.included_directories.emplace_back(
        path.get_text(),
        s_project->source_dir.get_text());
  }

  for (const auto &path : s_project->get_source_files())
  {
    processor.add_file({ path, SourceFileType::None });
  }

  return { Error::Ok };
}

ErrorReport ProjectService::SetupSourceProperties(SourceProcessor &processor) {
  s_source_io_map.clear();
  s_hanging_source_files.clear();
  s_hash_mismatched_source_files.clear();
  s_unrecorded_source_files.clear();

  for (const auto &inputs : processor.get_inputs())
  {
    s_source_files.emplace_back(inputs.path.resolved_copy());
    const auto &[emplaced_io, _] = s_source_io_map.emplace(
        s_source_files.back(),
        GetCompiledOutputPath(inputs.path, processor.get_file_hash(inputs.path))
            .resolved_copy());

    if (!emplaced_io->second.is_file() || emplaced_io->second.is_empty())
    {
      s_hanging_source_files.push_back(emplaced_io->first);
      continue;
    }

    auto obj_hash = build_tools::GetFileHash(emplaced_io->second.c_str());
    if (obj_hash == 0)
    {
      continue;
    }

    s_obj_files_hashes_map.emplace(emplaced_io->second, obj_hash);
    s_source2obj_files_hashes_map.emplace(emplaced_io->first, obj_hash);
  }

  for (const auto &[path, info] : processor.get_dependency_info_map())
  {
    s_source_files_hashes_map.emplace(path, info.hash);
  }

  for (const auto &path : s_source_files)
  {
    auto hash_iter = s_source_files_hashes_map.find(path);
    auto obj_hash_iter = s_source2obj_files_hashes_map.find(path);
    auto record_iter = s_current_cache.file_records.find(path);

    const bool has_hash = (hash_iter != s_source_files_hashes_map.end());
    const bool has_obj_hash =
        (obj_hash_iter != s_source2obj_files_hashes_map.end());
    const bool has_record = (record_iter != s_current_cache.file_records.end());

    if (!has_obj_hash)
    {
      s_hanging_source_files.push_back(path);
      Logger::verbose("source file has no valid cached object: %s",
                      path.c_str());
      continue;
    }

    if (!has_hash || !has_record)
    {
      s_unrecorded_source_files.push_back(path);
      Logger::verbose("source file not in records: %s", path.c_str());
      continue;
    }

    if (record_iter->second.hash != hash_iter->second)
    {
      s_hash_mismatched_source_files.push_back(path);
      Logger::verbose("source file hash mismatch: %s [%X -> %X]",
                      path.c_str(),
                      record_iter->second.hash,
                      hash_iter->second);
      continue;
    }

    if (record_iter->second.obj_hash != obj_hash_iter->second)
    {
      s_hash_mismatched_source_files.push_back(path);
      Logger::verbose("object file hash mismatch: %s [%X -> %X]",
                      path.c_str(),
                      record_iter->second.obj_hash,
                      obj_hash_iter->second);
      continue;
    }
  }

  return { Error::Ok };
}

ErrorReport ProjectService::PopulateSourceFilesToBuild() {
  for (const auto &path : s_hash_mismatched_source_files)
  {
    s_compile_needed_source_files.push_back(path);
  }
  for (const auto &path : s_unrecorded_source_files)
  {
    s_compile_needed_source_files.push_back(path);
  }
  for (const auto &path : s_hanging_source_files)
  {
    s_compile_needed_source_files.push_back(path);
  }
  return {};
}

ErrorReport ProjectService::SetupBuildCommand(
    const FilePath &source_path,
    build_tools::BuildCommandInfo &cmd_info) {
  const FilePath &output_path = s_source_io_map.at(source_path).resolved_copy();
  const hash_t &hash = s_source_files_hashes_map.at(source_path);
  // fully intended to not use tha `at()` function VVVVVVVVVVVVVVVVVV
  const hash_t &obj_hash = s_source2obj_files_hashes_map[source_path];

  const FileStats file_stats = { source_path };

  BuildCache::FileRecord record;
  record.hash = hash;
  record.obj_hash = obj_hash;
  record.output_path = output_path;
  record.source_write_time = file_stats.last_write_time.count();

  s_updated_cache.override_old_source_record(source_path, record);

  Logger::verbose("adding \"%s\" -> \"%s\" to the build force",
                  source_path.c_str(),
                  output_path.c_str());

  const SourceFileType file_type =
      s_project->get_source_type_or_default(source_path.get_text());

  Logger::verbose("source file type for '%s': %s",
                  source_path.c_str(),
                  build_tools::GetSourceFileTypeName(file_type));

  cmd_info.in_path = source_path;
  cmd_info.out_path = output_path;

  s_current_config->build_arguments(cmd_info.args,
                                    source_path.get_text(),
                                    output_path.get_text(),
                                    file_type);

  cmd_info.name = source_path.c_str();
  cmd_info.flags |= build_tools::eExcFlag_Printout;
  cmd_info.out = &std::cout;
  return {};
}

ErrorReport ProjectService::DispatchBuildCommands(int *output_codes) {
  /*
    diverting build output to independent streams, avoid parallel output
    shenanigans
  */

  const size_t count = s_used_build_commands.size();
  std::vector<std::ostringstream> build_output_streams{};
  build_output_streams.resize(count);

  const auto *_old_build_output_streams_data = build_output_streams.data();

  // setting up
  for (size_t i = 0; i < count; i++)
  {
    s_used_build_commands[i].out = build_output_streams.data() + i;
  }

  // building
  ErrorReport err =
      ExecuteBuildCommands(s_used_build_commands.data(), output_codes, count);
  if (err)
  {
    Logger::error(err);
    return err;
  }

  // unloading
  std::vector<string> names{};
  for (const auto &cmd : s_used_build_commands)
  {
    names.emplace_back(cmd.name);
  }

  err = DumpBuildCommandsOutoutStreams(build_output_streams.data(),
                                       names.data(),
                                       count);
  if (err)
  {
    Logger::error(err);
    return err;
  }

  err = ReportSourceBuildFailures();
  if (err)
  {
    Logger::error(err);
    return err;
  }

  if (_old_build_output_streams_data != build_output_streams.data())
  {
    Logger::error(
        "build's output streams vector was dislocated: old_data=%p, "
        "new_data=%p,"
        " the output streams are referenced by pointers that may now be "
        "dangling!"
        " (check if the above outputs are intelligible)",

        _old_build_output_streams_data,
        build_output_streams.data());
  }

  return {};
}

ErrorReport ProjectService::ExecuteBuildCommands(
    const build_tools::BuildCommandInfo *cmds,
    int *output_codes,
    size_t count) {
  const bool multithreaded =
      Settings::Get("build_multithreaded", false).get_bool();
  const int64_t threads_count =
      Settings::Get("build_jobs_count", (FieldVar::Int)8).get_int();

  const Blob<const build_tools::BuildCommandInfo> build_cmds_blob = { cmds,
                                                                      count };

  std::vector<int> result_codes = {};
  if (multithreaded)
  {
    result_codes =
        build_tools::Execute_Multithreaded(build_cmds_blob, threads_count);
  }
  else
  {
    result_codes = build_tools::Execute(build_cmds_blob);
  }

  LOG_ASSERT(result_codes.size() == count);
  std::copy(result_codes.begin(), result_codes.end(), output_codes);

  return {};
}

ErrorReport ProjectService::DumpBuildCommandsOutoutStreams(
    const std::ostringstream *streams,
    const string *names,
    size_t count) {
  constexpr auto whitespace_predicate = [](char chr) -> bool {
    return isspace(chr);
  };
  const bool report_silent_builds =
      Settings::Get("report_silent_builds", FieldVar(false)).get_bool();

  for (size_t i = 0; i < count; i++)
  {
    std::string output_str = streams[i].str();

    if (output_str.empty() ||
        std::all_of(output_str.begin(), output_str.end(), whitespace_predicate))
    {
      if (report_silent_builds)
      {
        Logger::notify("'%s' had no output.", names[i].c_str());
      }
      continue;
    }

    Logger::notify("'%s' output: ", names[i].c_str());

    Logger::raise_indent();

    {
      const std::string tmp = output_str;

      output_str = string_tools::replace<char>(
          tmp,
          "\n",
          std::string(Logger::_get_indent_str()) + '\n');
    }

    Logger::write_raw("%s", output_str.c_str());

    Logger::lower_indent();
  }

  return {};
}

ErrorReport ProjectService::ReportSourceBuildFailures() {
  const size_t compile_failures = GetBuildFailureCount();

  for (size_t i = 0; i < s_used_build_commands.size(); i++)
  {
    if (s_source_build_result_codes[i] == 0)
    {
      continue;
    }
    const auto &execute_params = s_used_build_commands[i];
    Logger::verbose("Building '%s' Failed with error code: %d",
                    execute_params.name.c_str(),
                    s_source_build_result_codes[i]);
  }

  if (compile_failures > 0)
  {
    Logger::warning("Linking will fail: %llu out of %llu builds have failed",
                    compile_failures,
                    s_used_build_commands.size());
  }

  return {};
}

void ProjectService::LoadObjectHashesToUpdatedCache() {
  for (const auto &[in_path, out_path] : s_source_io_map)
  {
    if (out_path.is_file())
    {
      s_updated_cache.file_records.at(in_path).obj_hash =
          build_tools::GetFileHash(out_path.c_str());
    }
  }
}

vector<StrBlob> ProjectService::GenerateLinkerInputs() {
  std::vector<StrBlob> result = {};

  for (const auto &[src_path, obj_path] : s_source_io_map)
  {
    result.emplace_back(obj_path.get_text());
  }

  return result;
}

void ProjectService::DumpBuildCommands(
    const build_tools::BuildCommandInfo *cmds,
    size_t count) {
  FilePath output_path = s_project->get_output().dir->join_path(".args");
  std::ofstream stream = output_path.stream_write(false);

  const std::streampos start = stream.tellp();

  build_tools::WriteAutoGeneratedHeader(stream);
  stream << "# below are the arguments for building the intermediates (object "
            "files) and "
            "linking/compiling final\n";

  // We can do it by converting 'map' to field var and save it
  // sadly, im too lazy rn

  for (size_t i = 0; i < count; i++)
  {
    bool first = true;
    for (const auto &arg : cmds[i].args)
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

  Logger::verbose("dumped the build argument list: size=%lld bytes to '%s'",
                  stream.tellp() - start,
                  output_path.c_str());
}
