#pragma once
#include "../Command.hpp"

namespace commands
{
	
	class HelpCommand : public Command
	{
	public:
		inline HelpCommand() : Command("help", "prints help messages") {
		}

		Error execute(ArgumentSource &reader) override;
		Error get_help(ArgumentSource &reader, string &out) override;
		inline CommandInfo get_info() const override {
			return {
				"help",
				"givens help on bgnu or on other commands"
			};
		}
	};

}
