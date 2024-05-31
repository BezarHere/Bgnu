#pragma once
#include "../Command.hpp"

namespace commands
{
	
	class HelpCommand : public Command
	{
	public:
		inline HelpCommand() : Command("help", "prints help messages") {
		}

		inline Error execute(ArgumentReader &reader) override {
			
			return Error::Ok;
		}
	};

}
