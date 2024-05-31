#include "Startup.hpp"

#include <iostream>

int main(int argc, const char *argv[]) {
	Startup::start(ArgumentReader::from_args(argv, argc));
}
