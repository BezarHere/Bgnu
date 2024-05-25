#include "Console.hpp"
#include "FieldFile.hpp"
#include <iostream>

int main() {
	// Console::set_fg(ConsoleColor::IntenseBlue);
	// Console::set_bg(ConsoleColor::Green);

	std::cout << "hello\n";

	FieldVar loaded_file = FieldFile::load("F:\\gcc\\bgnu\\test.txt");
	std::cout << loaded_file << '\n';

	FieldFile::dump("F:\\gcc\\bgnu\\dump.txt", loaded_file.get_dict());


	// FieldVar var = 312.41F;
	// FieldVar var2 = false;
	// FieldVar var3 = "yeah baby!";

	// FieldVar::Dict dict{{"age", var}, {"admin", var2}, {"bio", var}};
	// FieldVar var4{dict};

	// std::cout << "hello!\n";
	// std::cout << var << '\n';
	// std::cout << var2 << '\n';
	// std::cout << var3 << '\n';
	// std::cout << var4 << '\n';
}
