#include "Argument.hpp"



ArgumentReader ArgumentReader::from_args(const char_type *argv[], size_t argc) {
	ArgumentReader reader{};
	for (size_t i = 0; i < argc; i++)
	{
		reader.m_args.emplace_back(false, argv[i]);
	}

	return reader;
}

const Argument *ArgumentReader::get_next() {
	auto index = _find_unused();
	return &m_args[index];
}

const Argument *ArgumentReader::find_named(const string &name) {
	for (const Argument &arg : m_args)
	{
		if (arg.used)
		{
			continue;
		}

		return &arg;
	}

	return nullptr;
}
