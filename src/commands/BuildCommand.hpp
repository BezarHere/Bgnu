#pragma once
#include "Command.hpp"
#include "Project.hpp"
#include "BuildCache.hpp"
#include "code/SourceProcessor.hpp"
#include "misc/StaticString.hpp"

#include <map>
#include <set>

class SourceProcessor;

namespace commands
{

	class BuildCommand : public Command
	{
	public:
		typedef std::map<FilePath, FilePath> IOMap;

		struct ExecuteParameter
		{
			StaticString<128> name;
			std::vector<std::string> args;
		};

		struct FileInputOutput
		{
			FilePath input, output;
		};

		inline BuildCommand() : Command("build", "builds a project") {
		}

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
		void _load_properties(ArgumentReader &reader);
		void _load_project();
		void _setup_build_config(ArgumentReader &reader);
		void _write_build_info() const;
		bool _load_build_cache();

		void _build_source_processor();
		std::map<FilePath, FilePath> _gen_io_map() const;

		inline FilePath _get_build_cache_path() const {
			return m_project.get_output().dir->join_path(".build");
		}

		FilePath _get_output_filepath(const FilePath &filepath, const hash_t &hash) const;

		std::vector<int> _execute_build(const vector<ExecuteParameter> &params);

		static std::vector<StrBlob> _get_linker_inputs(const IOMap &io_map);

		static void _add_uncompiled_files(const commands::BuildCommand::IOMap &input_output_map,
																			const bool running_normal_build,
																			std::set<FilePath> &rebuild_files);

		void _add_changed_files(std::set<FilePath> &rebuild_files) const;
		bool _try_rebuild_setup();

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
