#include "Argument.hpp"


ArgumentReader::ArgumentReader(const char_type *argv[], size_t argc) {
	for (size_t i = 0; i < argc; i++)
	{
		m_args.emplace_back(argv[i], false);
	}
}

ArgumentReader::ArgumentReader(const Blob<const Argument> &args) : m_args{args.begin(), args.end()} {
}

Argument &ArgumentReader::read() {
	size_t index = _find_unused();

	if (index == npos)
	{
		throw std::runtime_error("no more arguments to read");
	}

	return m_args[index];
}

Argument *ArgumentReader::extract(const string &name) {
	for (size_t i = 0; i < m_args.size(); i++) {
		if (m_args[i].is_used())
		{
			continue;
		}

		if (m_args[i].value == name) {
			return &m_args[i];
		}
	}
	return nullptr;
}

Argument *ArgumentReader::extract_any(const Blob<const string> &names) {
	for (const string &name : names)
	{
		Argument *arg = extract(name);

		if (arg == nullptr)
		{
			continue;
		}

		return arg;
	}
	return nullptr;
}
