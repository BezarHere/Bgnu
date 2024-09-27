#include "Startup.hpp"

#include <iostream>
#include "Logger.hpp"
#include "FilePath.hpp"
#include <Windows.h>

namespace chrono
{
	using namespace std::chrono;
}

int main(int argc, const char *argv[]) {
	const chrono::time_point pre_run_time = chrono::steady_clock::now();

	int result = Startup::start(ArgumentReader(argv + 1, std::max(argc - 1, 0)));

	const auto run_time = chrono::duration_cast<chrono::milliseconds>(
		chrono::steady_clock::now() - pre_run_time
	);

	Logger::debug(
		"-- run time: %.2fs --",
		(float)run_time.count() / (float)decltype(run_time)::period::den
	);

	return result;
}
