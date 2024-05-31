#include "FieldFile.hpp"
#include "Project.hpp"
#include "Command.hpp"

#include <iostream>

int main(int argc, const char *argv[]) {
	CommandDB::_load_commands();
	ArgumentReader reader = ArgumentReader::from_args(argv, argc);

	std::cout << "hello\n";

	std::cout << std::boolalpha;

	Glob glob = "**\\*.[ch][px][px]";
	
	string path;
	std::cin >> path;

	std::cout << "\n\"" << path << "\" matches as " << glob.test(path) << '\n';

	FieldVar loaded_file = FieldFile::load("F:\\gcc\\bgnu\\test.txt");
	ErrorReport report{};
	Project project = Project::from_data(loaded_file.get_dict(), report);

	std::cin.get();
}
