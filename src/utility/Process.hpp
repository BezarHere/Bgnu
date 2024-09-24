#pragma once
#include <string>

class Process
{
public:
	typedef char char_type;
	Process(int argc, const char_type *const *argv);
	Process(const char_type *cmd);

	int start(std::ostream *out = nullptr);
;
private:
	std::string m_cmd;
};
