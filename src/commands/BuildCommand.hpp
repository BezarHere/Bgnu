#pragma once
#include "Command.hpp"
#include "Project.hpp"
#include "BuildCache.hpp"

#include <regex>

class SourceProcessor;

namespace commands
{

	class BuildCommand : public Command
	{
	public:
		inline BuildCommand() : Command("build", "builds a project") {
		}

		Error execute(ArgumentReader &reader) override;

		static FilePath _default_filepath();

	private:
		void _write_build_cache() const;
		void _load_build_cache();

		void _build_source_processor(SourceProcessor &processor);

		inline FilePath _get_build_cache_path() const {
			return m_project.get_output().cache_dir.join_path(".build");
		}

	private:
		bool m_rebuild = false;
		bool m_resave = false;

		Project m_project;
		
		const string_char *m_current_build_cfg_name = nullptr;
		const BuildConfiguration *m_current_build_cfg;

		ErrorReport m_report;
		BuildCache m_build_cache;
	};

}
