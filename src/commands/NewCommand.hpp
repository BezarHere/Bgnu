#pragma once
#include "../Command.hpp"

namespace commands
{
	
	class NewCommand : public Command
	{
	public:
		inline NewCommand() : Command("new", "creates a new project") {
		}

		inline Error execute(ArgumentReader &reader) override {
			
			return Error::Ok;
		}
	};

}
