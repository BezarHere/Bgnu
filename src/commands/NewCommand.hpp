#pragma once
#include "../Command.hpp"

namespace commands
{

	class NewCommand : public Command
	{
	public:
		inline NewCommand() : Command("new", "creates a new project") {
		}

		Error execute(ArgumentReader &reader) override;
		Error get_help(ArgumentReader &reader, string &out) override;
		inline CommandInfo get_info() const override {
			return {
				"new",
				"create a new and default '.bgnu' project file in the working directory"
			};
		}
	};

}
