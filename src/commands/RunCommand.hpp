#pragma once
#include "../Command.hpp"

namespace commands
{
	
	class RunCommand : public Command
	{
	public:
		inline RunCommand() : Command("run", "runs the project after building") {
		}

		Error execute(ArgumentSource &reader) override;
		Error get_help(ArgumentSource &reader, string &out) override;
		inline CommandInfo get_info() const override {
			return {
				"run",
				"runs the project after building"
			};
		}
	};

}
