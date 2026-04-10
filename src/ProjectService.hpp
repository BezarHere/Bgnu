#pragma once
#include <cstdint>
#include <memory>
#include <sstream>

#include "Argument.hpp"
#include "BuildCache.hpp"
#include "BuildConfiguration.hpp"
#include "BuildTools.hpp"
#include "FilePath.hpp"
#include "Project.hpp"
#include "base.hpp"
#include "code/SourceProcessor.hpp"
#include "misc/Error.hpp"
#include "misc/hash128.hpp"

enum class BuildStep : uint8_t {
  None,
  ArgsSetup,
  ProjectSetup,
  CacheLoading,
  PrebuildProcessing,
  SourceInfoLoading,
  OldCacheClearing,
  UpdatingCache,
  PopulatingCompileList,
  PopulateBuildCommands,
  PostBuildCommandsPopulating,
  BuildCommandsDispatching,
  UpdateBuildCacheApplying,
  Linking,
  PostLinking
};

class ProjectService
{
public:
  ProjectService() = delete;

  static void Clear();

  static Error ExecuteStep(BuildStep step);

  // returns the error code + step failed
  // or `{Error::Ok, BuildStep::None}` on success
  static std::pair<Error, BuildStep> ExecuteStepsTo(BuildStep final_step);

  static void SetArguments(ArgumentSource args);

  // STEPS FOR BUILDING
  static Error SetupArgs();
  static Error SetupProject();
  static Error LoadCaches();
  static Error ProcessPrebuild();
  static Error LoadSourceInfo();
  static Error RemoveUnusedCacheFiles();
  static Error UpdateBuildCache();
  static Error PopulateCompileNeededFiles();
  static Error PopulateBuildCommands();
  static Error PostBuildCommandsStep();  // <- clangd stuff
  static Error DispatchBuildProcesses();
  static Error ReapplyUpdatedBuildCache();
  static Error LinkBuiltFiles();
  static Error PostLinkingStep();

  static Error DumpAvailableBuildCommands();

  static Project *GetProject() { return s_project.get(); }
  static const BuildConfiguration *GetBuildConfig() { return s_current_config; }
  static const string &GetBuildConfigName() { return s_current_config_name; }

  static const FilePath &GetProjectFile() { return s_project_file; }
  static const FilePath &GetBuildDir() { return s_build_directory; }

  static inline bool IsCacheLoaded() { return s_cache_loaded; }
  static inline bool IsRebuildRequired() { return s_forced_rebuild; }
  static inline bool ShouldRebuild() {
    return s_forced_rebuild || !s_cache_loaded || s_hash_mismatched;
  }

  static inline bool IsFullBuild() {
    return s_final_step == BuildStep::PostLinking;
  }

  static inline BuildStep GetStep() { return s_current_step; }

  static inline FilePath GetBuildCachePath() {
    return s_project->get_output().dir->join_path(".build");
  }

  static FilePath GetCompiledOutputPath(const FilePath &path, hash_t hash);
  static size_t GetBuildFailureCount();
  static size_t GetBuildSuccessCount();
  static inline bool IsBuildSuccessful() { return GetBuildFailureCount() == 0; }

  static const char *GetStepName(BuildStep step);
  static FilePath GetLinkOutputPath();
  static std::string GetDefaultConfigName();

  static void UpdateHashMismatchFlag();
  static bool IsProjectHashMatching();
  static bool IsConfigHashMatching();

private:
  static Error ExecuteStep_Inner(BuildStep step);

  static ErrorReport LoadProject();
  static ErrorReport SetupConfigArgs(ArgumentSource &src);
  static ErrorReport SetupConfig();

  static ErrorReport ReadBuildCache();
  static ErrorReport BuildSourceProcessor(SourceProcessor &processor);
  static ErrorReport SetupSourceProperties(SourceProcessor &processor);

  static ErrorReport PopulateSourceFilesToBuild();

  static ErrorReport SetupBuildCommand(const FilePath &source_path,
                                       build_tools::BuildCommandInfo &cmd_info);

  static ErrorReport DispatchBuildCommands(int *output_codes);
  static ErrorReport ExecuteBuildCommands(
      const build_tools::BuildCommandInfo *cmds,
      int *output_codes,
      size_t count);
  static ErrorReport DumpBuildCommandsOutoutStreams(
      const std::ostringstream *streams,
      const string *names,
      size_t count);

  static ErrorReport ReportSourceBuildFailures();

  static void LoadObjectHashesToUpdatedCache();

  static vector<StrBlob> GenerateLinkerInputs();
  static void DumpBuildCommands(const build_tools::BuildCommandInfo *cmds,
                                size_t count);

private:
  static BuildStep s_current_step;
  static BuildStep s_final_step;
  static bool s_read_only;

  static ArgumentSource s_arguments;

  static std::unique_ptr<Project> s_project;
  static string s_current_config_name;
  static const BuildConfiguration *s_current_config;

  static vector<FilePath> s_source_files;
  // new file hash is different
  static vector<FilePath> s_hash_mismatched_source_files;
  // not in the build cache record
  static vector<FilePath> s_unrecorded_source_files;
  // no object file found
  static vector<FilePath> s_hanging_source_files;
  static vector<FilePath> s_compile_needed_source_files;
  static vector<build_tools::BuildCommandInfo> s_total_build_commands;
  static vector<build_tools::BuildCommandInfo> s_used_build_commands;
  static vector<int> s_source_build_result_codes;
  static std::map<FilePath, FilePath> s_source_io_map;
  static std::map<FilePath, hash_t> s_source_files_hashes_map;
  static std::map<FilePath, hash_t> s_obj_files_hashes_map;
  static std::map<FilePath, hash_t> s_source2obj_files_hashes_map;

  static build_tools::BuildCommandInfo s_linking_build_cmd;
  static int s_linking_result_code;

  static FilePath s_project_file;
  static FilePath s_build_directory;

  static BuildCache s_current_cache;
  static BuildCache s_updated_cache;

  static bool s_cache_loaded;
  static bool s_forced_rebuild;
  static bool s_resave_required;
  static bool s_hash_mismatched;
};
