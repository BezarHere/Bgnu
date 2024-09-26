#pragma once
#include <array>
#include <string>

class Process
{
public:
	static constexpr size_t PrintableCMDLength = 31;
	typedef char char_type;
	typedef std::array<char_type, PrintableCMDLength + 1> PrintableBuffer;

	Process(int argc, const char_type *const *argv);
	Process(const char_type *cmd);
	Process(const std::string &cmd);

	int start(std::ostream *out = nullptr);

	inline const char_type *get_printable_cmd() const {
		return m_printable_cmd.data();
	}

private:
	static PrintableBuffer _BuildPrintableCMD(const char *string);

private:
	std::string m_cmd;
	PrintableBuffer m_printable_cmd = {0};
};
