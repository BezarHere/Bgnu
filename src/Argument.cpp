#include "Argument.hpp"

ArgumentSource::ArgumentSource(const char_type *argv[], size_t argc) {
	for (size_t i = 0; i < argc; i++)
	{
		m_args.emplace_back(argv[i], false);
	}
}

ArgumentSource::ArgumentSource(const Blob<const Argument> &args) : m_args{args.begin(), args.end()} {
}

string ArgumentSource::join() const {
  string result = {};
  for (const auto &arg : m_args)
  {
    result.append("\"");
    result.append(arg.get_value());
    result.append("\" ");
  }

  if (!result.empty())
  {
    result.pop_back();
  }

  return result;
}

Argument &ArgumentSource::read() {
	size_t index = _find_unused();

	if (index == npos)
	{
		throw std::runtime_error("no more arguments to read");
	}

  m_args[index].mark_used();

	return m_args[index];
}

const std::string &ArgumentSource::read_or(const std::string &default_value) {
	size_t index = _find_unused();

	if (index == npos)
	{
		return default_value;
	}

	m_args[index].mark_used();

	return m_args[index].get_value();
}

size_t ArgumentSource::find(const string &name) const {
  for (size_t i = 0; i < m_args.size(); i++)
  {
    if (!m_args[i].is_used() && m_args[i].get_value() == name)
    {
      return i;
    }
  }

  return npos;
}

Argument *ArgumentSource::extract(const string &name) {
	for (size_t i = 0; i < m_args.size(); i++) {
		if (m_args[i].is_used())
		{
			continue;
		}

		if (m_args[i].get_value() == name) {
			return &m_args[i];
		}
	}
	return nullptr;
}

Argument *ArgumentSource::extract_any(const Blob<const string> &names) {
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

bool ArgumentSource::check_flag(const string &name) {
	Argument *arg = extract(name);
	if (arg)
	{
		arg->mark_used();
	}

	return arg != nullptr;
}

bool ArgumentSource::check_flag_any(const Blob<const string> &names) {
	Argument *arg = extract_any(names);
	if (arg)
	{
		arg->mark_used();
	}

	return arg != nullptr;
}

void ArgumentSource::simplify() {

	// reverse iterate to reduce moving values
	for (size_t i = 1; i <= m_args.size(); i++)
	{
		const size_t rindex = m_args.size() - i;

		if (m_args[rindex].is_used())
		{
			m_args.erase(m_args.begin() + static_cast<ptrdiff_t>(rindex));

			i--;
		}
	}

}
