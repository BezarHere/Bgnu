#include "FieldFile.hpp"
#include "Project.hpp"
#include "Command.hpp"

#include <iostream>

int main(int argc, const char *argv[]) {
	CommandDB::_load_commands();
	ArgumentReader reader = ArgumentReader::from_args(argv, argc);

	std::cout << "hello\n";

	std::cout << std::boolalpha;

	Glob glob = "**\\bgnu\\test.*";
	std::cout << glob.test("F:\\gcc\\bgnu\\test.txt") << '\n';
	std::cout << glob.test("F:\\gcc\\bgnu\\test.gg") << '\n';
	std::cout << glob.test("\\bgnu\\test.gg") << '\n';
	std::cout << glob.test("bgnu\\test.gg") << '\n';

	FieldVar loaded_file = FieldFile::load("F:\\gcc\\bgnu\\test.txt");
	ErrorReport report{};
	Project project = Project::from_data(loaded_file.get_dict(), report);

	std::cout << &project << '\n';

	std::cout << "now break here to see how is `project`" << '\n';
	std::cin.get();
}
