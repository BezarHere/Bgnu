#include "HelpCommand.hpp"
#include "Logger.hpp"


Error commands::HelpCommand::execute(ArgumentReader &reader) {
	size_t commands_length = 0;
	const CommandDB::command_ptr *commands = CommandDB::get_commands(commands_length);

	for (size_t i = 0; i < commands_length; i++)
	{
		const CommandInfo info = commands[i]->get_info();
		Logger::notify("%s", info.name.c_str());
		Logger::raise_indent();
		Logger::notify("%s", info.desc.c_str());
		std::string str = "";
		ArgumentReader reader = {{nullptr}};

		commands[i]->get_help(reader, str);
		// str.replace("\n", std::string("\n") + Logger::_get_indent_str());
		Logger::write_raw("%s\n", str.c_str());

		Logger::lower_indent();
	}

	return Error();
}

Error commands::HelpCommand::get_help(ArgumentReader &reader, string &out) {
	out.append(get_info().desc);
	return Error::Ok;
}
