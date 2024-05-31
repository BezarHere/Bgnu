#include "Command.hpp"

#include <map>
#include "Logger.hpp"
#include "StringUtils.hpp"

#include "commands/HelpCommand.hpp"
#include "commands/BuildCommand.hpp"
#include "commands/NewCommand.hpp"

constexpr size_t MaxCommandsCount = 8;

struct CommandDB::Data
{
	size_t commands_count;
	array<command_ptr, MaxCommandsCount> commands;
};

CommandDB::Data CommandDB::s_data;

Command *CommandDB::get_command(const string &name) {
	for (size_t i = 0; i < s_data.commands_count; i++)
	{
		if (s_data.commands[i]->name == name)
		{
			return s_data.commands[i].get();
		}
	}

	return nullptr;
}

CommandDB::CommandSuggestion CommandDB::get_suggestion(const string &name) {
	const string lower_name = StringUtils::to_lower(name);

	CommandDB::CommandSuggestion suggestion;
	suggestion.confidence = 0.0F;
	suggestion.command = nullptr;

	for (size_t i = 0; i < s_data.commands_count; i++)
	{
		float similarity = StringUtils::similarity(lower_name, s_data.commands[i]->name);
		if (similarity > suggestion.confidence)
		{
			suggestion.confidence = similarity;
			suggestion.command = s_data.commands[i].get();
			if (similarity == 1.0F)
			{
				break;
			}
		}
	}

	return suggestion;
}

const CommandDB::command_ptr *CommandDB::get_commands(size_t &length) {
	length = s_data.commands_count;
	return s_data.commands.data();
}

void CommandDB::_load_commands() {
	_add_command(std::make_unique<commands::HelpCommand>());
	_add_command(std::make_unique<commands::BuildCommand>());
	_add_command(std::make_unique<commands::NewCommand>());
}

void CommandDB::_add_command(command_ptr &&command) {
	if (s_data.commands_count >= MaxCommandsCount)
	{
		Logger::error("CommandDB: too many commands; can not add command at %p", command.get());
		return;
	}

	s_data.commands[s_data.commands_count++].reset(command.release());
}