#include "Startup.hpp"

#include <iostream>
#include "Logger.hpp"
#include "FilePath.hpp"

int main(int argc, const char *argv[]) {
	for (int i = 0; i < argc; i++)
	{
		Logger::debug("arg no.%d: '%s'", i, argv[i]);
	}

	return Startup::start(ArgumentReader(argv + 1, std::max(argc - 1, 0)));
}
