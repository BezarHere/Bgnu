#pragma once
#include "base.hpp"

struct Argument
{
	bool used = false;
	const string value = "";
};

class ArgumentReader
{
public:
	using char_type = string::value_type;

	static ArgumentReader from_args(const char_type *argv[], size_t argc);

	const Argument *get_next();

	const Argument *find_named(const string &name);

	inline size_t count() const { return m_args.size(); }

	inline const vector<Argument> &get_args() const { return m_args; }
	inline size_t get_cursor() const { return m_cursor; }

	inline void reset_cursor() { m_cursor = 0; }

	inline ArgumentReader slice(size_t start, size_t end) const {
		ArgumentReader reader{};
		for (size_t i = start; i < end; i++)
		{
			reader.m_args.emplace_back(m_args[i]);
		}
		return reader;
	}

	inline ArgumentReader slice(size_t start) const { return slice(start, m_args.size()); }

private:
	inline size_t _find_unused() const {
		for (size_t i = m_cursor; i < m_args.size(); i++)
		{
			if (!m_args[i].used)
			{
				return i;
			}
		}
		return npos;
	}

private:
	size_t m_cursor = 0;
	vector<Argument> m_args;
};
