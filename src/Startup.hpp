#pragma once
#include "Argument.hpp"
#include "FieldVar.hpp"

class Startup
{
public:
	static int start(ArgumentReader reader);

private:
	static void _build_env();
	static void _check_misspelled_command(const string &name);


private:
	static FieldVar::Dict s_env;
};
