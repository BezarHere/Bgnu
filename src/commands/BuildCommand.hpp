#pragma once
#include "Command.hpp"
#include "Project.hpp"
#include "BuildCache.hpp"
#include "code/SourceProcessor.hpp"

#include <map>

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

		inline BuildCommand() : Command("build", "builds a project") {
		}

		Error execute(ArgumentReader &reader) override;

		static FilePath _default_filepath();

	private:
		void _load_properties(ArgumentReader &reader);
		void _load_project();
		void _setup_build_config(ArgumentReader &reader);
		void _write_build_cache() const;
		void _load_build_cache();

		void _build_source_processor();
		std::map<FilePath, FilePath> _gen_io_map() const;

		inline FilePath _get_build_cache_path() const {
			return m_project.get_output().dir.join_path(".build");
		}

		FilePath _get_output_filepath(const FilePath &filepath, const hash_t &hash) const;

		void _execute_build(const vector<vector<string>> &args);

		static std::vector<StrBlob> _get_linker_inputs(const IOMap &io_map);

	private:
		bool m_rebuild = false;
		bool m_resave = false;

		Project m_project;
		FilePath m_project_file;

		string m_current_build_cfg_name;
		const BuildConfiguration *m_current_build_cfg = nullptr;
		SourceProcessor m_src_processor;

		ErrorReport m_report;
		BuildCache m_build_cache;
	};

}
