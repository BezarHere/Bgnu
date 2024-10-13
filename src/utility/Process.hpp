#pragma once
#include "misc/StaticString.hpp"
#include <array>
#include <string>

class Process
{
public:
	static constexpr size_t PrintableCMDLength = 63;
	typedef char char_type;
	typedef StaticString<PrintableCMDLength> ProcessName;

	Process(int argc, const char_type *const *argv);
	Process(const char_type *cmd);
	Process(const std::string &cmd);

	int start(std::ostream *out = nullptr);

	inline const ProcessName &get_name() const {
		return m_name;
	}

	inline void set_name(const ProcessName &name) {
		m_name = name;
	}

private:
	static ProcessName _BuildPrintableCMD(const char *string);

private:
	std::string m_cmd;
	ProcessName m_name = {};
	unsigned long m_wait_time_ms = 1000 * 60; // 1 minutes
};
