#include "Process.hpp"
#include "base.hpp"
#include "StringTools.hpp"

#include <algorithm>

#include <sstream>

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE Pipe;
#elif __unix__
// TODO: unix stuff

typedef _Filet *Pipe;
struct _ProcessData
{
	pid_t handle;
};
#endif

static string join_args(const Blob<const Process::char_type *const> &args);

Process::Process(int argc, const char_type *const *argv)
	: Process(join_args({argv, argc}).c_str()) {
}

Process::Process(const char_type *cmd)
	: m_cmd{cmd} {
}

int Process::start(std::ostream *const out) {
	int exit_code = -1;
	Pipe output = {};

#ifdef _WIN32

	if (out)
	{
		CreatePipe(
			&output, nullptr, nullptr, 0
		);
	}

	STARTUPINFOA startup_info = {};
	startup_info.cb = sizeof(startup_info);
	startup_info.dwFlags = STARTF_USESTDHANDLES;
	startup_info.hStdOutput = output;
	startup_info.hStdError = output;

	PROCESS_INFORMATION process_info = {};

	SECURITY_ATTRIBUTES sec_attrs = {};
	sec_attrs.nLength = sizeof(sec_attrs);

	std::string cmd_copy = m_cmd;
	CreateProcessA(
		nullptr, cmd_copy.data(),
		&sec_attrs, nullptr,
		TRUE, CREATE_NO_WINDOW,
		nullptr, nullptr,
		&startup_info, &process_info
	);

	(void)WaitForSingleObject(
		process_info.hProcess, INFINITE
	);

	for (uint32_t i = 0; i < 64; i++)
	{
		if (GetExitCodeProcess(process_info.hProcess, reinterpret_cast<DWORD *>(&exit_code)) == 0)
		{
			continue;
		}
		
		printf("exit code: %d\n", exit_code);
		break;
	}

	// // make sure the process is killed?
	// CloseHandle(process_info.hProcess);

	if (output && out)
	{
		constexpr size_t buffer_size = 1024;
		char buffer[buffer_size + 1] = {};
		DWORD read_sz = 0;

		while (::ReadFile(output, buffer, buffer_size, &read_sz, nullptr))
		{
			if (read_sz == 0)
			{
				break;
			}

			buffer[read_sz] = 0;
			(*out) << buffer;
		}

	}

	CloseHandle(output);

#endif


	return exit_code;
}

string join_args(const Blob<const Process::char_type *const> &args) {
	std::ostringstream oss;

	for (const Process::char_type *arg : args)
	{
		bool need_enclosure = string_tools::all_of(
			arg,
			[](Process::char_type value) {return !isgraph(value);}
		);

		if (need_enclosure)
		{
			oss << '"' << arg << '"';
		}
		else
		{
			oss << arg;
		}

		oss << ' ';
	}

	return oss.str();
}
