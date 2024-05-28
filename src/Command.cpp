#include "Command.hpp"

#include <map>
#include "Logger.hpp"

constexpr size_t MaxCommandsCount = 8;

struct CommandDB::Data
{
	size_t commands_count;
	array<command_ptr, MaxCommandsCount> commands;
};

CommandDB::Data CommandDB::s_data;

const CommandDB::command_ptr *CommandDB::get_commands(size_t &length) {
	length = s_data.commands_count;
	return s_data.commands.data();
}

void CommandDB::_load_commands() {
}

void CommandDB::_add_command(command_ptr &&command) {
	if (s_data.commands_count >= MaxCommandsCount)
	{
		Logger::error("CommandDB: too many commands; can not add command at %p", command.get());
		return;
	}

	s_data.commands[s_data.commands_count++].reset(command.release());
}