#pragma once
#include "Argument.hpp"
#include "misc/Error.hpp"
#include <memory>

class Command
{
public:
	inline Command(const string &_name, const string &_desc)
		: name{_name}, description{_desc} {
	}

	virtual ~Command() = default;

	virtual Error execute(ArgumentReader &reader) = 0;

	inline virtual Error get_help(ArgumentReader &reader, string &help_str) {
		(void)reader;
		help_str = "";
		return Error::NotImplemented;
	}

	const string name;
	const string description;

private:
	Command(const Command &) = delete;
	Command &operator=(const Command &) = delete;
};

class CommandDB
{
public:
	using command_ptr = std::unique_ptr<Command>;
	
	struct CommandSuggestion
	{
		Command *command;
		float confidence;
	};

	static Command *get_command(const string &name);
	static CommandSuggestion get_suggestion(const string &name);

	static const command_ptr *get_commands(size_t &length);

	static void _load_commands();

private:
	static void _add_command(command_ptr &&command);

private:
	struct Data; 
	static Data s_data;
};
