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
	};

}
