#include "Console.hpp"
#include "ProjFile.hpp"
#include <iostream>

int main() {
	// Console::set_fg(ConsoleColor::IntenseBlue);
	// Console::set_bg(ConsoleColor::Green);

	std::cout << "hello\n";

	ProjVar loaded_file = ProjFile::load("F:\\gcc\\bgnu\\test.txt");
	std::cout << loaded_file << '\n';

	ProjFile::dump("F:\\gcc\\bgnu\\dump.txt", loaded_file.get_dict());


	// ProjVar var = 312.41F;
	// ProjVar var2 = false;
	// ProjVar var3 = "yeah baby!";

	// ProjVar::VarDict dict{{"age", var}, {"admin", var2}, {"bio", var}};
	// ProjVar var4{dict};

	// std::cout << "hello!\n";
	// std::cout << var << '\n';
	// std::cout << var2 << '\n';
	// std::cout << var3 << '\n';
	// std::cout << var4 << '\n';
}
