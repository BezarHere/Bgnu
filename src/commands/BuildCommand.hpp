#pragma once
#include "../Command.hpp"

namespace commands
{
	
	class BuildCommand : public Command
	{
		inline BuildCommand() : Command("build", "builds a project") {
		}

		inline Error execute(ArgumentReader &reader) override {
			
		}
	};

}