#pragma once
#include "Command.hpp"
#include "Project.hpp"
#include "BuildCache.hpp"
#include "code/SourceProcessor.hpp"
#include "misc/StaticString.hpp"

#include "BuildTools.hpp"

#include <map>
#include <set>

class SourceProcessor;

namespace commands
{

  class BuildCommand : public Command
  {
  public:
    typedef std::map<FilePath, FilePath> IOMap;


    struct FileInputOutput
    {
      FilePath input, output;
    };

    inline BuildCommand() : Command("build", "builds a project") {}

    Error execute(ArgumentReader &reader) override;
    Error get_help(ArgumentReader &reader, string &out) override;
    inline CommandInfo get_info() const override {
      return {
        "build",
        "builds the project in the working directory, project files are named '.bgnu'"
      };
    }

    bool is_running_rebuild() const;

    static FilePath _default_filepath();

  private:
    Error _setup_build(ArgumentReader &reader);

    void _load_properties(ArgumentReader &reader);
    void _load_project();
    void _setup_build_config(ArgumentReader &reader);
    void _write_build_info() const;
    bool _load_build_cache();

    std::vector<int> _do_build_on(build_tools::ExecuteParameter *build_args, size_t count) const;

    size_t _check_report_compile_failures(const int *build_results,
                                          const build_tools::ExecuteParameter *build_args,
                                          size_t count) const;

    void _process_build_source(const commands::BuildCommand::IOMap &input_output_map,
                               const FilePath &source_path,
                               std::vector<build_tools::ExecuteParameter> &build_args);

    void _build_source_processor();
    std::map<FilePath, FilePath> _gen_io_map() const;

    inline FilePath _get_build_cache_path() const {
      return m_project.get_output().dir->join_path(".build");
    }

    FilePath _get_output_filepath(const FilePath &filepath, const hash_t &hash) const;

    static std::vector<StrBlob> _get_linker_inputs(const IOMap &io_map);

    static void _add_uncompiled_files(const commands::BuildCommand::IOMap &input_output_map,
                                      const bool running_normal_build,
                                      std::set<FilePath> &rebuild_files);

    void _add_changed_files(std::set<FilePath> &rebuild_files) const;
    bool _check_rebuild_required();

    static std::vector<int> ExecuteBuild(const Blob<const ::build_tools::ExecuteParameter> &params);

  private:
    bool m_rebuild = false;
    bool m_resave = false;
    bool m_loaded_build_cache = false;

    Project m_project;
    FilePath m_project_file;

    string m_current_build_cfg_name;
    const BuildConfiguration *m_current_build_cfg = nullptr;
    SourceProcessor m_src_processor;

    ErrorReport m_report;
    BuildCache m_build_cache;
    BuildCache m_new_cache;
  };

}
