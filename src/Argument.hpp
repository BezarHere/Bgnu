#pragma once
#include "base.hpp"

struct Argument
{

	inline Argument(const string &value, bool used = false)
		: m_value{value}, m_used{used} {
	}

	inline void mark_used() { m_used = true; }
	inline bool is_used() const { return m_used; }
	inline const string &get_value() const { return m_value; }

	/// @brief tries to use the argument (marking it as 'used')
	/// @returns weather the argument can be used (false if it's already used or if this is null)
	/// @note can be used on a nullptr, altho, it will returns false
	static inline bool try_use(Argument *arg) {
		if (arg == nullptr)
		{
			return false;
		}

		if (arg->m_used)
		{
			return false;
		}

		arg->mark_used();

		return true;
	}

	/// @returns the value of this argument, or `default_value` if this == nullptr
	/// @note can be used on a nullptr, altho, it will returns false
	static inline const string &try_get_value(const Argument *arg, const string &default_value = "") {
		if (arg == nullptr)
		{
			return default_value;
		}

		return arg->m_value;
	}

private:
	string m_value = "";
	bool m_used = false;
};

class ArgumentSource
{
public:
	using char_type = string::value_type;

	ArgumentSource(const char_type *argv[], size_t argc);
	ArgumentSource(const Blob<const Argument> &args);

  inline bool is_empty() const { return _find_unused() == npos; }

  string join() const;

	Argument &read();
	const std::string &read_or(const std::string &default_value);

  // returns npos if not found, does not check used arguments
  size_t find(const string &name) const;

	Argument *extract(const string &name);
	Argument *extract_any(const Blob<const string> &names);

	// returns true if the flag is found, false otherwise
	// marks the argument with the flags as used
	bool check_flag(const string &name);

	// returns true if a flag with a matched name is found, false otherwise
	// marks the argument matched as used
	bool check_flag_any(const Blob<const string> &names);

	template <typename _Pred>
	inline Argument *extract_matching(_Pred &&pred) {
		for (size_t i = 0; i < m_args.size(); i++)
		{
			if (m_args[i].is_used())
			{
				continue;
			}

			if (pred(const_cast<const Argument &>(m_args[i])))
			{
				return &m_args[i];
			}
		}
		return nullptr;
	}

	size_t count() const {
		size_t counter = 0;
		for (const auto &arg : m_args)
		{
			if (!arg.is_used())
			{
				counter++;
			}
		}
		return counter;
	}

	inline const vector<Argument> &get_args() const { return m_args; }

	inline ArgumentSource slice(size_t start, size_t end) const {
		return ArgumentSource({m_args.data() + start, m_args.data() + end});
	}

	inline ArgumentSource slice(size_t start) const { return slice(start, m_args.size()); }

	// removes all the used arguments
	void simplify();

private:
	inline size_t _find_unused() const {
		for (size_t i = 0; i < m_args.size(); i++)
		{
			if (!m_args[i].is_used())
			{
				return i;
			}
		}
		return npos;
	}

private:
	vector<Argument> m_args;
};
