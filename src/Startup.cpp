#include "Startup.hpp"
#include "Logger.hpp"
#include "Command.hpp"
#include "FieldFile.hpp"
#include "Settings.hpp"

FieldVar::Dict Startup::s_env = {};

static void StartupMessage();

int Startup::start(ArgumentSource reader) {
	setlocale(LC_ALL, "en_US.utf8");

	StartupMessage();
	Settings::Init();
	Settings::SaveBackup();

	if (reader.is_empty())
	{
		Logger::warning("no command passed, please pass a command and it's arguments");
		return 1;
	}

	_build_env(reader);

	reader.simplify();

	CommandDB::_load_commands();

	Argument &command_arg = reader.read();
	command_arg.mark_used();

	Command *command = CommandDB::get_command(command_arg.get_value());

	if (command == nullptr)
	{
		Logger::error("No command named '%s'", to_cstr(command_arg.get_value()));
		_check_misspelled_command(command_arg.get_value());
		return 1;
	}

	ArgumentSource command_input = reader.slice(1);

	Console::push_state();
	Logger::_push_state();

	Error error;

	// try
	// {
	error = command->execute(command_input);
	// }
	// catch (std::exception &e)
	// {
	// 	Logger::error(
	// 		"Command '%s' has raised an exception: what='%s'",
	// 		to_cstr(command->name),
	// 		to_cstr(e.what())
	// 	);

	// 	throw;
	// }

	Logger::_pop_state();
	Console::pop_state();

	if (error != Error::Ok)
	{
		Logger::error("Command '%s' failed, returned error %d", to_cstr(command->name), (int)error);
	}
	else
	{
		const bool temp = Settings::s_SilentSaveFail;
		Settings::s_SilentSaveFail = true;
		Settings::Save(true);
		Settings::s_SilentSaveFail = temp;
	}

	return (int)error;
}

void Startup::_build_env(ArgumentSource &reader) {
	static const string VerboseArgs[] = {"-v", "--verbose"};

	if (Argument::try_use(reader.extract_any(VerboseArgs)))
	{
		Logger::_make_verbose();
	}

	s_env.emplace("verbose", Logger::is_verbose());

	s_env.emplace("command_min_confidence", 0.65F);

	string env_str = FieldFile::write(s_env);
	Logger::debug("Enviornment variables: %s", to_cstr(env_str));
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

	if (suggestion.confidence < (float)s_env.at("command_min_confidence"))
	{
		return;
	}

	Logger::note("Maybe you meant '%s'?", to_cstr(suggestion.command->name));
}

void StartupMessage() {
	const auto version = Settings::GetVersion();

	Logger::notify(
		"BGnu version %u.%u.%u",
		version.major,
		version.minor,
		version.patch
	);

	Logger::notify(
		"Zahr abdullatif babker (@Bezar/@BezarHere) (C) 2024 - 2025"
	);
}
