#include "Startup.hpp"

#include <iostream>
#include "Logger.hpp"

int main(int argc, const char *argv[]) {
	for (int i = 0; i < argc; i++)
	{
		Logger::debug("arg no.%d: '%s'", i, argv[i]);
	}

	Startup::start(ArgumentReader::from_args(argv + 1, std::max(argc - 1, 0)));
}
