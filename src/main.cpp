#include "Startup.hpp"

#include <iostream>
#include "Logger.hpp"
#include "FilePath.hpp"

int main(int argc, const char *argv[]) {
	for (int i = 0; i < argc; i++)
	{
		Logger::debug("arg no.%d: '%s'", i, argv[i]);
	}

	// string path;
	// std::cin >> path;

	// FilePath fpath{path};

	// std::cout << '\n' << "original: " << fpath << '\n';
	// fpath.resolve();
	// std::cout << '\n' << "resolved: " << fpath << '\n';
	
	return Startup::start(ArgumentReader(argv + 1, std::max(argc - 1, 0)));
}
