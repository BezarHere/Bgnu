#pragma once
#include "../Command.hpp"
#include "../Project.hpp"

#include <regex>

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
		void _load_build_cache();

		inline FilePath _get_build_cache_path() const {
			return m_project.get_output().cache_dir.join_path(".build");
		}

	private:
		bool m_rebuild = false;
		bool m_resave = false;

		Project m_project;
		ErrorReport m_report;
		BuildCache m_build_cache;
	};

}
