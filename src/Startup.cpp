#include "Startup.hpp"
#include "Logger.hpp"
#include "Command.hpp"

FieldVar::Dict Startup::s_env = {};

void Startup::start(ArgumentReader reader) {
	if (reader.count() == 0)
	{
		Logger::warning("no command passed, please pass a command and it's arguments");
		return;
	}


	_build_env();
	CommandDB::_load_commands();

	const Argument *command_arg = reader.get_next();

	Command *command = CommandDB::get_command(command_arg->value);

	if (command == nullptr)
	{
		Logger::error("No command named '%s'", to_cstr(command_arg->value));
		_check_misspelled_command(command_arg->value);
		return;
	}

}

void Startup::_build_env() {
	s_env.emplace("min_broadcast_conf", 0.65F);
}

void Startup::_check_misspelled_command(const string &name) {
	volatile auto suggestion = CommandDB::get_suggestion(name);



	if (suggestion.command == nullptr)
	{
		Logger::debug("Suggestion: no command matched even slightly");
		return;
	}

	Logger::debug(
		"Suggestion: command_name=\"%s\", confidence=%0.2f",
		to_cstr(suggestion.command->name),
		suggestion.confidence
	);

	if (suggestion.confidence < (float)s_env.at("min_broadcast_conf"))
	{
		return;
	}

	Logger::note("Maybe you meant '%s'?", to_cstr(suggestion.command->name));
}
