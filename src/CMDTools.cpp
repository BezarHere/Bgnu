#include "CMDTools.hpp"
#include <thread>


int CMDTools::execute(const string_char *cmd_line) {
	return system(cmd_line);
}

void CMDTools::execute_pool(const Blob<string> &commands, vector<int> &ret_codes) {
	std::thread *threads = new std::thread[commands.size];

	for (size_t i = 0; i < commands.size; i++)
	{
		threads[i] = std::thread(
			[&ret_codes](const string_char *cmd) { ret_codes.push_back(execute(cmd)); },
			commands[i].c_str()
		);
	}

	for (size_t i = 0; i < commands.size; i++)
	{
		threads[i].join();
	}

	delete[] threads;
}
